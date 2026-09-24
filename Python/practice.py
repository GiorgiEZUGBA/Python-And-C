from datetime import timedelta, timezone, datetime
from fastapi import FastAPI, Body, Depends, status, Path, HTTPException
from fastapi.security import OAuth2PasswordBearer, OAuth2PasswordRequestForm
from fastapi.middleware.cors import CORSMiddleware
from typing import Annotated
from pwdlib import PasswordHash
from sqlmodel import Field, SQLModel

from sqlalchemy.ext.asyncio import create_async_engine, AsyncSession
from sqlalchemy import text

from pydantic_settings import BaseSettings, SettingsConfigDict 

import jwt
from jwt.exceptions import InvalidTokenError

from dotenv import load_dotenv
import os

class Settings(BaseSettings):
    DATABASE_URL: str
    SECRET_KEY: str
    ALGORITHM: str
    TOKEN_EXPIRATION_MINUTES: int

    model_config = SettingsConfigDict(env_file=".env", env_file_encoding="utf-8")

settings = Settings()

# MODELS(ENTITIES)
class BaseUser(SQLModel):
    id: int | None = Field(default = 9999, primary_key=True)
    name: str | None = Field(default = None, max_length=100, min_length=3)
    age: int = Field(ge = 18)
    department: str = Field(default="Sales")

class PublicUser(BaseUser):
    pass

class CreateUser(BaseUser, table = True):
    password: str = Field(max_length= 60, min_length= 8)

class Report(SQLModel, table = True):
    id: int = Field(ge = 1, primary_key=True)
    text: str | None = None

# JWT MODELS

class Token(SQLModel):
    access_token: str
    token_type: str

class TokenData(SQLModel):
    username: str


# SQL CONNECTION
engine = create_async_engine(settings.DATABASE_URL)

async def get_db():
    async with AsyncSession(engine) as session:
        try:
            yield session
        finally:
            pass


#Application
app = FastAPI()
password_hash = PasswordHash.recommended()

oauth2_scheme = OAuth2PasswordBearer(tokenUrl="token")

origins = [
    "http://localhost.tiangolo.com",
    "https://localhost.tiangolo.com",
    "http://localhost",
    "http://localhost:8080"
]

app.add_middleware(
    CORSMiddleware,
    allow_origins = origins,
    allow_credentials = True,
    allow_methods = ["*"],
    allow_headers = ["*"]
)
    

# FUNCTIONS
async def db_get_users(db: AsyncSession) -> list[PublicUser]:
    result = await db.execute(text("EXEC proc_GetAllUsers"))
    return result.mappings().all()

async def db_create_user(user: Annotated[CreateUser, Body()], db : AsyncSession):
    query = text("EXEC proc_Create_New_User @Id = :Id, @Username = :Username, " \
    "@Age  = :Age, @Department = :Department, @Password = :Password")
    params = {"Username" : user.name, "Id" : user.id, "Age" : user.age, 
              "Department" : user.department, "Password" : password_hash.hash(user.password)}
    
    await db.execute(query, params)
    await db.commit()
    return {"status" : "executed in T-Sql"}

async def db_get_report(report_id: int, db: AsyncSession):
    if report_id == 999:
        raise ValueError("T-SQL Error: Timeout or Deadlock")
    query = text("EXEC proc_getReprotById @Id = :Id")
    params = {"Id" : report_id}
    result = await db.execute(query, params)
    return result.mappings().first()


async def db_get_user_by_name(username: str, db: AsyncSession):
    query = text("EXEC proc_GetUserByName @Username = :Username")
    result = await db.execute(query, {"Username": username})
    return result.mappings().first()

# JWT FUNCTIONS
def create_token(data: dict, expires_delta: timedelta | None = None):
    to_encode = data.copy()
    expiration = datetime.now(timezone.utc) + timedelta(minutes=15)
    if expires_delta:
        expiration = datetime.now(timezone.utc) + expires_delta
    to_encode.update({"exp" : expiration})
    token = jwt.encode(to_encode, settings.SECRET_KEY, algorithm=settings.ALGORITHM)
    return token

async def get_current_user(token: Annotated[str, Depends(oauth2_scheme)], 
                     db: Annotated[AsyncSession, Depends(get_db)]):
    credentials_exception = HTTPException(
        status_code=status.HTTP_401_UNAUTHORIZED,
        detail="Invalid token",
        headers={"WWW-Authenticate": "Bearer"},
    )

    try:
        token_info = jwt.decode(token, settings.SECRET_KEY, algorithms=[settings.ALGORITHM])
        username = token_info["sub"]
        if not username:
            raise credentials_exception 
    except InvalidTokenError:
        raise credentials_exception
    
    user = await db_get_user_by_name(username, db)
    if user is None:
        raise credentials_exception
    return PublicUser(**user)
    


# MAPPINGS
@app.post("/token", response_model=Token)
async def login(
    form_data: Annotated[OAuth2PasswordRequestForm, Depends()],
    db: Annotated[AsyncSession, Depends(get_db)]
):
    user = await db_get_user_by_name(form_data.username, db)
    
    if not user or not password_hash.verify(form_data.password, user["password"]):
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Inccorect username or password",
            headers={"WWW-Authenticate": "Bearer"},
        )
        
    token_expires = timedelta(minutes=settings.TOKEN_EXPIRATION_MINUTES)
    token = create_token(
        data={"sub": form_data.username}, expires_delta=token_expires
    )
    return {"access_token": token, "token_type": "bearer"}

@app.get("/users", response_model=list[PublicUser])
async def get_users(db: Annotated[AsyncSession, Depends(get_db)]) -> list[PublicUser]:
    all_users = await db_get_users(db)
    return all_users


@app.post("/users")
async def create_user(user: Annotated[CreateUser, Body()], 
                      db: Annotated[AsyncSession, Depends(get_db)]) :
    result = await db_create_user(user, db)
    return {"status_code" : status.HTTP_201_CREATED, "detail" : result}


@app.get("/reports/{reportId}", response_model=Report)
async def get_report(reportId : Annotated[int, Path()],
                     db: Annotated[AsyncSession, Depends(get_db)],
                     current_user: Annotated[PublicUser, Depends(get_current_user)]) -> Report:
    try:
        report = await db_get_report(reportId, db)
        if not report:
            raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Report not found")
        return report
    except ValueError as e:
        raise HTTPException(status_code=status.HTTP_400_BAD_REQUEST, detail=str(e))
