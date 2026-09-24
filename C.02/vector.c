#include "vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void VectorNew(vector *v, int elemSize, VectorFreeFunction freeFn, int initialAllocation){
    assert(initialAllocation >= 0);
    if(initialAllocation == 0){
        initialAllocation = 4;
    } 
    v->logLen = 0;
    v->allLen = initialAllocation;
    v->memSize = elemSize;
    v->elems = malloc(v->allLen * v->memSize);
    v->freeFun = freeFn;
    assert(v->elems != NULL);
}

void VectorDispose(vector *v){
    if (v->freeFun != NULL) {
        for (int i = 0; i < v->logLen; i++) {
            v->freeFun(VectorNth(v, i));
        }
    }
    free(v->elems);
}

int VectorLength(const vector *v){
    return v->logLen;
}

void *VectorNth(const vector *v, int position){
    assert(position < v->logLen && position >= 0);
    void* ptr = (char*)v->elems + position * v->memSize;  
    return ptr; 
}

void VectorReplace(vector *v, const void *elemAddr, int position){
    assert(position < v->logLen && position >= 0);
    void* ptr = (char*)v->elems + position * v->memSize;
    memcpy(ptr, elemAddr, v->memSize);
}

void VectorInsert(vector *v, const void *elemAddr, int position){
    assert(position <= v->logLen && position >= 0);
    if(v->logLen == position){
        VectorAppend(v, elemAddr);
        return;
    }
    if(v->logLen == v->allLen) {
        v->allLen *= 2;
        v->elems = realloc(v->elems, v->allLen * v->memSize);
        assert(v->elems != NULL);
    }
    void* startPtr = (char*)v->elems + position * v->memSize;
    void* endPtr =  (char*)v->elems + (position + 1) * v->memSize;
    memmove(endPtr, startPtr, (v->logLen - position) * v->memSize);
    v->logLen++;
    VectorReplace(v, elemAddr, position);
}

void VectorAppend(vector *v, const void *elemAddr){
    if(v->logLen == v->allLen) {
        v->allLen *= 2;
        v->elems = realloc(v->elems, v->allLen * v->memSize);
        assert(v->elems != NULL);
    }
    void* ptr = (char*)v->elems + v->logLen * v->memSize;
    memcpy(ptr, elemAddr, v->memSize);
    v->logLen++;
}

void VectorDelete(vector *v, int position){
    assert(position <= v->logLen - 1 && position >= 0);
    if(position == v->logLen - 1){
        v->logLen--;
        return;
    }
    void* startPtr = (char*)v->elems + position * v->memSize;
    void* endPtr = (char*)v->elems + (position + 1) * v->memSize;
    memmove(startPtr, endPtr, (v->logLen - position - 1) * v->memSize);
    v->logLen--;
}

void VectorSort(vector *v, VectorCompareFunction compare){
    assert(compare != NULL);
    qsort(v->elems, v->logLen, v->memSize, compare); // first i impelemented search function than i did not know that binary search and quick sort were real functions 
}

void VectorMap(vector *v, VectorMapFunction mapFn, void *auxData){
    assert(mapFn != NULL);
    for(int i = 0; i < v->logLen; i++){
        void* ptr = (char*)v->elems + i * v->memSize;
        mapFn(ptr, auxData);
    }
}

static const int kNotFound = -1;

int VectorSearch(const vector *v, const void *key, VectorCompareFunction searchFn, int startIndex, bool isSorted){
    assert(startIndex >= 0 && startIndex <= v->logLen);
    assert(key != NULL);
    assert(searchFn != NULL);
    if(isSorted){
        int miN = startIndex;
        int maX = v->logLen - 1;
        while(miN <= maX){
            int pos = (miN + maX) / 2;
            void* ptr = (char*)v->elems + pos * v->memSize;
            int cmp = searchFn(ptr, key);
            if(cmp == 0){
                return pos;
            } else if(cmp > 0){
                maX = pos - 1;
            } else {
                miN = pos + 1;
            }
        }
    } else {
        for(int i = startIndex; i < v->logLen; i++){
            void* ptr = (char*)v->elems + i * v->memSize;
            if(searchFn(ptr, key) == 0){
                return i;
            }
        }
    }
    return kNotFound; 
} 
