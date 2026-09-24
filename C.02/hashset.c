#include "hashset.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void HashSetNew(hashset *h, int elemSize, int numBuckets,
		HashSetHashFunction hashfn, HashSetCompareFunction comparefn, HashSetFreeFunction freefn){
	h->memSize = elemSize;
	h->logLen = 0;
	h->numBucks = numBuckets;
	h->hashFun = hashfn;
	h->cmpFun = comparefn;
	h->freeFun = freefn;
	h->buckets = malloc(h->numBucks * sizeof(vector));
	for(int i = 0; i < h->numBucks; i++){
		VectorNew(h->buckets + i, elemSize, h->freeFun, 4);
	}
	assert(h->memSize > 0 && h->numBucks > 0 && h->hashFun != NULL && h->cmpFun != NULL);
}

void HashSetDispose(hashset *h){
	for(int i = 0; i < h->numBucks; i++){
		vector* ptr = h->buckets + i;
		VectorDispose(ptr);
	}
	free(h->buckets);
}

int HashSetCount(const hashset *h){
	return h->logLen;
}

void HashSetMap(hashset *h, HashSetMapFunction mapfn, void *auxData){
	assert(mapfn != NULL);
    for(int i = 0; i < h->numBucks; i++){
        vector* ptr = h->buckets + i;
		for(int j = 0; j < ptr->logLen; j++){
			void* elem = VectorNth(ptr, j);
			mapfn(elem, auxData);
		}
    }
}

void HashSetEnter(hashset *h, const void *elemAddr){
	assert(elemAddr != NULL);
	int buckIdx = h->hashFun(elemAddr, h->numBucks);
	assert(buckIdx >= 0 && buckIdx < h->numBucks);
	vector* bucket = h->buckets + buckIdx;
	int pos = VectorSearch(bucket, elemAddr, h->cmpFun, 0, false);
	if(pos != -1){
		VectorReplace(bucket, elemAddr, pos);
	} else {
		VectorAppend(bucket, elemAddr);
		h->logLen++;
	}
}

void *HashSetLookup(const hashset *h, const void *elemAddr){
	assert(elemAddr != NULL);
	int buckIdx = h->hashFun(elemAddr, h->numBucks);
	assert(buckIdx >= 0 && buckIdx < h->numBucks);
	vector* bucket = h->buckets + buckIdx;
	int pos = VectorSearch(bucket, elemAddr, h->cmpFun, 0, false);
	if(pos == -1){
		return NULL;
	}
	return VectorNth(bucket, pos);
}
