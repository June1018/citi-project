-sysocbsi (commbuff_t *cb, int index, void *data, long size)
commbuff에 지정된 slot에 데이터 저장
size가 0이거나 data null이면 기존 존재하는 데이터 clear 해당 index에 이미 데이터가 존재하는 경우 size가 같으면 기존데이터 replace

-sysocbai(commbuff_t *cb, int index, void *data, long size) 
commbuff에 지정된slot에 데이터 append한다. 

-sysocbci(commbuff_t *cb, int index) 
commbuff에 지정된 slot에 데이터를 clear한다.

-sysocbgp(commbuff_t *cb, int index)
commbuff에 지정된slot에buffer pointer를 얻는다.

-sysocbgs(commbuff_t *cb, int index)
commbuff에 지정된 slot의 buffer size를 얻는다.
 
-sysocbsb(commbuff_t *cb, void *buf, long size)
commbuff에 항목별 pointer를 set한다. 
 
-sysocbdb(commbuff_t *cb, commbuff_t *dcb) 
commbuff를 복사한다.

-sysocbrb(commbuff_t *cb, commbuff_t *mcb)
commbuff를 Restore한다. 

-sysocbfb(commbuff_t *cb) 
commbuff를 free한다.



