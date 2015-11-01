#include <string.h>
#include "MemoryAllocator.h"

#ifdef _MSC_VER

#include <Windows.h>
typedef double max_align_t;

#else//_MSC_VER

#include <unistd.h>

#endif//_MSC_VER

typedef union AreaHeader {
	struct {
		union AreaHeader*	nextFree;
		size_t				size;
	} s;

	max_align_t _;
} AreaHeader;

static const size_t defaultSize	= 1024*1024;
static const size_t limitSize	= 1024*1024*1024;
static const size_t	alignment	= _Alignof(max_align_t);
static const size_t headerSize	= sizeof(AreaHeader);

static char*		memory;
static size_t		allocated;
static size_t		pageSize = 4096;
static AreaHeader*	freeListBase;
static AreaHeader*	freeList;


//	バッファの伸長システムコール
static void* AllocateBuffer(size_t size) {
#ifdef _MSC_VER

	if(!memory) {
		//	ページサイズの取得
		SYSTEM_INFO info;
		GetSystemInfo(&info);

		pageSize = info.dwPageSize;

		//	確保
		memory = VirtualAlloc(NULL, limitSize, MEM_RESERVE, PAGE_NOACCESS);
	}

	char* ptr = memory + allocated;
	allocated += size;

	if(!memory || allocated >= limitSize) {
		return NULL;
	}
	
	return VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);

#else//_MSC_VER
	
	void* p = sbrk(size);
	allocated += size;

	if(p == (void*)-1)
		return NULL;
	else if(memory)
		return memory + allocated - size;
	else
		return memory = p;

#endif//_MSC_VER
}


//	次のブロック
static AreaHeader* NextArea(AreaHeader* header) {
	return (AreaHeader*)((char*)(header+1) + header->s.size);
}


//	メモリの確保
void* MemoryAllocate(size_t size) {
	if(!memory) {
		//	初回
		freeListBase = freeList	= AllocateBuffer(defaultSize);
		freeList->s.nextFree	= freeList;
		freeList->s.size		= defaultSize - headerSize;
	}

	//	アライメント要求に揃える
	size = size % alignment == 0
		? size
		: (size / alignment + 1) * alignment;

	if(freeList) {
		//	空き領域を検索
		AreaHeader* current	= freeList;
		AreaHeader* prev	= freeList;
	
		do {
			freeList = freeList->s.nextFree;
	
			if(freeList->s.size > headerSize + size) {
				//	十分大きいブロックが見つかった
				//	ブロックの後ろ部分を返す
				freeList->s.size -= headerSize + size;
	
				AreaHeader* block	= NextArea(freeList);
				block->s.size		= size;
	
				return block + 1;
			}
			else if(freeList->s.size >= size) {
				//	だいたいピッタリのが見つかった
				//	見つかったブロックを返す
				AreaHeader* block = freeList;
				
				//	リストの中をすっぱ抜く
				prev->s.nextFree = freeList->s.nextFree;
				
				//	イテレータを動かしておく
				if(freeList == freeList->s.nextFree)
					freeList = NULL;
				else
					freeList = freeList->s.nextFree;

				//	先頭要素を確保した時は先頭位置を変更
				if(block == freeListBase)
					freeListBase = freeList;
	
				return block + 1;
			}
	
			prev = freeList;
		} while(freeList != current);
	}

	//	見つからなかったあるいは領域がない
	size_t		len = ((headerSize + size) / pageSize + 1) * pageSize;
	AreaHeader*	ptr = AllocateBuffer(len);

	if(!ptr)
		return NULL;

	//	リストに追加
	len -= headerSize + size;
	ptr->s.size = len;
	MemoryFree(ptr + 1);

	ptr = NextArea(ptr);
	ptr->s.size = size;

	return ptr + 1;
}


//	領域サイズの変更
void* MemoryReallocate(void* ptr, size_t size) {
	if(!ptr)
		return MemoryAllocate(size);

	//	念のための範囲チェック
	//	管理範囲外とのポインタ比較演算は処理系依存であることに注意
	if((char*)ptr < memory || (char*)ptr <= memory)
		return NULL;

	if(size == 0) {
		MemoryFree(ptr);
		return NULL;
	}

	//	メモリブロック
	AreaHeader* block = (AreaHeader*)ptr - 1;

	//	アライメント要求に揃える
	size = size % alignment == 0
		? size
		: (size / alignment + 1) * alignment;

	if(size + headerSize < block->s.size) {
		//	十分に小さい
		//	ブロックを縮めてそのまま引数を帰す
		size_t s = block->s.size;
		block->s.size = size;
		
		AreaHeader* a = NextArea(block);
		a->s.size = s - size - headerSize;

		MemoryFree(a);

		return ptr;
	}
	else if(size <= block->s.size) {
		//	だいたい大きさが変わってない
		return ptr;
	}
	else {
		//	より大きい
		//	新しく確保してコピー、古いのは解放
		void* p = MemoryAllocate(size);
		memcpy(p, ptr, block->s.size);
		
		MemoryFree(ptr);

		return p;
	}
}


//	解放
void MemoryFree(void* ptr) {
	if(!ptr)
		return;

	//	念のための範囲チェック
	//	管理範囲外とのポインタ比較演算は処理系依存であることに注意
	if((char*)ptr < memory || (char*)ptr <= memory)
		return;

	//	ブロックの取得
	AreaHeader* block = (AreaHeader*)ptr - 1;

	if(freeList) {
		AreaHeader* p = freeListBase;

		do {
			if(p < block && block < p->s.nextFree) {
				//	リストの間
				AreaHeader* next = p->s.nextFree;

				if(NextArea(block) == next) {
					//	後のブロックと隣接してる
					block->s.nextFree = next->s.nextFree;
					block->s.size += headerSize + next->s.size;

					if(freeList == next)
						freeList = block;
				}
				else {
					//	後のブロックとは隣接していない
					block->s.nextFree = next;
				}

				if(NextArea(p) == block) {
					//	前のブロックと隣接してる
					p->s.nextFree = block->s.nextFree;
					p->s.size += headerSize + block->s.size;
				}
				else {
					//	前のブロックと隣接していない
					p->s.nextFree = block;
				}

				return;
			}
			
			if(p->s.nextFree == freeListBase) {
				//	今一番後ろの要素を参照している

				if(block < freeListBase) {
					//	先頭のブロックよりも前
					if(NextArea(block) == freeListBase) {
						//	先頭のブロックに隣接してる
						block->s.nextFree = freeListBase->s.nextFree;
						block->s.size += headerSize + freeListBase->s.size;
					}
					else {
						//	先頭のブロックには隣接していない
						block->s.nextFree = freeListBase;
					}
					
					if(freeList == freeListBase)
						freeList = block;

					//	先頭の位置をずらす
					freeListBase = block;
					p->s.nextFree = freeListBase;
				}
				else {
					//	末尾のブロックよりも後ろ
					if(NextArea(p) == block) {
						//	末尾のブロックと隣接している
						p->s.size += headerSize + block->s.size;
					}
					else {
						//	末尾のブロックとは隣接していない
						p->s.nextFree = block;
						block->s.nextFree = freeListBase;
					}
				}

				return;
			}

			p = p->s.nextFree;
		} while(p != freeListBase);
	}
	else {
		//	空き領域がない
		//	返却されたブロックをそのままリストに
		freeList = freeListBase = block;
		freeList->s.nextFree = freeList;
	}
}


//	ダンプ
void MemoryDump(FILE* file) {
	if(!file)
		return;

	//	取り敢えず解放済み領域だけ出力しておく
	fputs("=== MemoryDump ==================\n", file);
	fprintf(file, "mem:%p\tlen:%llu\n", memory, (unsigned long long)allocated);

	if(freeListBase) {
		AreaHeader* p = freeListBase;

		do {
			fprintf(file, "%p ~ %p : %llu\n", p, NextArea(p), (unsigned long long)p->s.size);
			p = p->s.nextFree;
		} while(p != freeListBase);
	}

	fputs("=================================\n", file);
}
