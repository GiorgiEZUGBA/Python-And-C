using namespace std;
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include "imdb.h"

const char *const imdb::kActorFileName = "actordata";
const char *const imdb::kMovieFileName = "moviedata";

imdb::imdb(const string& directory)
{
  const string actorFileName = directory + "/" + kActorFileName;
  const string movieFileName = directory + "/" + kMovieFileName;
  
  actorFile = acquireFileMap(actorFileName, actorInfo);
  movieFile = acquireFileMap(movieFileName, movieInfo);
}

bool imdb::good() const
{
  return !( (actorInfo.fd == -1) || 
	    (movieInfo.fd == -1) ); 
}

// you should be implementing these two methods right here... 
bool imdb::getCredits(const string& player, vector<film>& films) const { 
  int numActors = *(int*)actorFile;
  int* posArray = (int*)actorFile + 1;

  int currPos;
  char* currAct;
  int maX = numActors - 1;
  int miN = 0;

  while(true){
    if(miN > maX) {
      return false;
    }
    currPos = (maX + miN) / 2;
    currAct = (char*)actorFile + *(posArray + currPos);
    int compare = strcmp(player.c_str(), currAct); ;

    if(compare == 0){
      break;
    } else if(compare > 0){
      miN = currPos + 1; 
    } else {
      maX = currPos - 1;
    }
  }

  int idx = 0;
  while(*(currAct + idx) != '\0'){
    idx++;
  }
  idx++;
  if(idx % 2 != 0){
    idx++;
  }
  short filmNum = *(short*)(currAct + idx); 
  idx += 2;
  if(idx % 4 != 0){
    idx += 2;
  }

  int* filmArr = (int*)(currAct + idx);
  for(int i = 0; i < filmNum; i++) {
    char* movie = (char*)movieFile + *(filmArr + i);
    string title = movie;
    int yearIdx = title.size() + 1;
    int year = *(movie + yearIdx) + 1900;
    films.push_back({title, year});
  }

  return true;
} 

bool imdb::getCast(const film& movie, vector<string>& players) const { 
  int numFilms = *(int*)movieFile;
  int* posArray = (int*)movieFile + 1;

  int currPos;
  char* currFilm;
  int maX = numFilms - 1;
  int miN = 0;
  int idx;

  while(true){
    if(miN > maX){
      return false;
    }
    currPos = (maX + miN) / 2;
    currFilm = (char*)movieFile + *(posArray + currPos);
    int compare = strcmp(movie.title.c_str(), currFilm);

    if(compare == 0){
      idx = 0;
      while(*(currFilm + idx) != '\0'){
        idx ++;
      }
      idx++;
      int year = *(currFilm + idx) + 1900;
      idx++;

      if(year == movie.year){
        break;
      } else if(movie.year > year){
        miN = currPos + 1;
      } else {
        maX = currPos - 1;
      }

    } else if(compare > 0){
      miN = currPos + 1;
    } else {
      maX = currPos - 1;
    }

  }

  if(idx % 2 != 0){
    idx++;
  }
  short numAct = *(short*)(currFilm + idx);
  idx += 2;
  if(idx % 4 != 0){
    idx += 2;
  }
    
  int* actArr = (int*)(currFilm + idx);
  for(int i = 0; i < numAct; i++){
    char* actor = (char*)actorFile + *(actArr + i);
    players.push_back(actor);
  }

  return true;
}

imdb::~imdb()
{
  releaseFileMap(actorInfo);
  releaseFileMap(movieInfo);
}

// ignore everything below... it's all UNIXy stuff in place to make a file look like
// an array of bytes in RAM.. 
const void *imdb::acquireFileMap(const string& fileName, struct fileInfo& info)
{
  struct stat stats;
  stat(fileName.c_str(), &stats);
  info.fileSize = stats.st_size;
  info.fd = open(fileName.c_str(), O_RDONLY);
  return info.fileMap = mmap(0, info.fileSize, PROT_READ, MAP_SHARED, info.fd, 0);
}

void imdb::releaseFileMap(struct fileInfo& info)
{
  if (info.fileMap != NULL) munmap((char *) info.fileMap, info.fileSize);
  if (info.fd != -1) close(info.fd);
}