#ifndef INCLUDE_MEMORYALLOCATOR_H
#define INCLUDE_MEMORYALLOCATOR_H

#include <stddef.h>
#include <stdio.h>

/**
 * メモリの動的確保を行う
 * @param size 確保する領域のバイトサイズ
 * @return     領域の確保に失敗した場合、あるいはsizeが0の時はNULL<br/>
 *             そうでなければ確保された領域の先頭を指すポインタを返す<br/>
 *             返り値がNULLでない時、領域はあらゆるアライメント要求を満たす
 */
void* MemoryAllocate(size_t size);


/**
 * 確保されたメモリの領域のサイズを変更する<br/>
 * サイズ変更後の領域が小さくなる場合、変更後の領域外のデータは破棄される<br/>
 * サイズ変更後の領域が大きくなる場合、変更前のデータはコピーされる
 * @param ptr  サイズを変更する領域の先頭を指すポインタ<br/>
 *             NULLならばMemoryAllocate(size)を呼び出す<br/>
 *             MemoryAllocateで確保された領域の先頭を指さない非NULLポインタ<br/>
 *             あるいは既に解放された領域のポインタが渡された場合の動作は未定義
 * @param size 変更後の領域のサイズ
 * @return     領域の確保に失敗した時、あるいはsizeが0の時はNULL<br/>
 *             そうでなければ確保された領域の先頭を指すポインタを返す<br/>
 *             返り値がNULLでない時、領域はあらゆるアライメント要求を満たす
 */
void* MemoryReallocate(void* ptr, size_t size);


/**
 * 確保されたメモリを解放する
 * @param ptr MemoryAllocateで確保された領域の先頭を指すポインタ<br/>
 *            NULLならば何もしない<br/>
 *            MemoryAllocateで確保された領域の先頭を指さない非NULLポインタ、<br/>
 *            あるいは既に解放された領域のポインタが渡された場合の動作は未定義
 */
void MemoryFree(void* ptr);


/**
 * 現在のメモリ領域の内容をファイルに出力する<br/>
 * デバッグ用途なので適当
 * @param file 出力対象のファイルポインタ<br/>
 *             NULLなら何もしない
 */
void MemoryDump(FILE* file);

#endif//INCLUDE_MEMORYALLOCATOR_H
