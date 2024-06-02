# 1. 개요

본 과제는 (1) xv6의 메모리 관리 및 페이지 할당 과정을 분석한 다음 페이지 할당을 lazy allocation 방식으로 바꾸고, (2) xv6의 파일 시스템 구조를 분석한 다음 최대 1GB 크기의 파일을 저장할 수 있도록 3-level indirect block을 추가하는 실습 과제이다.

과제 4-1은 ssualloc() 시스템 콜을 통해 페이지 할당 요청을 받으면 가상 메모리만 할당한 다음, 그 메모리에 접근할 때 page fault trap을 처리하는 과정에서 실제 물리 메모리를 할당받는 방식의 lazy allocation을 구현해야 한다. 또한, 지금까지 할당된 가상 메모리 페이지의 개수와 물리 메모리 페이지의 개수를 구하는 getvp()와 getpp() 시스템 콜도 구현해야 한다.

과제 4-2는 기존에 12개의 direct block과 1개의 1-level indirect block으로 구성되어 있어 140개의 블록을 저장할 수 있었던 xv6의 파일 시스템을 6개의 direct block, 4개의 1-level indirect block, 2개의 2-level indirect block, 1개의 3-level direct block을 갖도록 수정해서 2130438개의 블록을 저장할 수 있도록 변경해야 한다.

두 과제를 수행하는 과정에서 xv6의 페이지 할당 과정과 파일 시스템을 분석하고, 강의에서 배운 이론을 실제로 코드를 작성하면서 복습할 수 있다.

# 2. 상세 설계

### 2-1. 페이지 할당 과정

기존의 페이지 할당 과정은 sbrk 시스템 콜의 구현을 보면 알 수 있다. sys_sbrk는 할당받을 크기 n을 인자로 받은 다음, 새로 할당되는 공간의 첫 주소인 myproc()->sz를 addr에 저장하고, growproc(n)을 호출한 뒤 addr을 반환한다.

growproc 함수는 현재 프로세스의 메모리를 증가시킬 크기인 n을 받은 다음, allocuvm 함수를 이용해 실제 공간을 할당받는다. 공간이 성공적으로 할당되면 myproc()->sz는 n 만큼 증가한다.

### 2-2. lazy allocation 구현

ssualloc 시스템 콜을 호출했을 때 가상 메모리 페이지만 할당하고 물리 메모리 페이지를 할당하지 않는 것은 간단하다. sbrk 시스템 콜에서 growproc을 호출하지 않고 단순히 myproc()->sz만 n 만큼 증가시키면 된다.

메모리에 접근할 때 실제 물리 메모리를 할당하는 것 trap에서 처리해야 한다. 할당되지 않은 메모리에 접근하면 T_PGFLT trap이 발생하므로 trap 함수의 switch-case문에서 T_PGFLT에 대한 처리를 추가하면 된다. 이 부분은 allocuvm에서 하는 것처럼 mappages 함수를 이용해 할당받은 공간을 페이지 테이블에 매핑하면 되는데, 이 과정에서 rcr2() 함수를 이용해 실제로 fault가 발생한 주소를 구한 뒤 PGROUNDDOWN 매크로를 이용해 페이지의 시작 주소를 알아내는 것이 필요하다,

### 2-3. getvp(), getpp() 구현

getvp()는 단순히 myproc()->sz / PGSIZE를 반환하면 된다. 각 프로세스마다 아직 물리 메모리 페이지가 할당되지 않은 가상 메모리 페이지의 개수 vp를 관리하면, getpp()는 myproc()->sz / PGSIZE – myproc()->vp를 반환하면 된다. 두 시스템 콜 모두 상수 시간에 동작한다.

### 2-4. struct dinode 변경

fs.h 파일을 보면, direct block의 개수를 나타내는 매크로 상수 NDIRECT와 indirect block에서 저장할 수 있는 block의 개수를 나타내는 매크로 상수 NINDIRECT가 있고, struct dinode 내에 블록의 주소를 나타내는 addrs[NDIRECT+1]이 선언되어 있다.

과제 명세에 나와있는 것처럼 파일 구조를 변경하기 위해서는 다음과 같이 코드를 수정해야 한다.

```c
#define NINDIRECT (BSIZE / sizeof(uint))

#define NDIRECT 6
#define NINDIRECT_1 4
#define NINDIRECT_2 2
#define NINDIRECT_3 1
#define MAXFILE (NDIRECT + NINDIRECT * NINDIRECT_1 + NINDIRECT * NINDIRECT * NINDIRECT_2 + NINDIRECT * NINDIRECT * NINDIRECT * NINDIRECT_3)

// On-disk inode structure
struct dinode {
  short type;           // File type
  short major;          // Major device number (T_DEV only)
  short minor;          // Minor device number (T_DEV only)
  short nlink;          // Number of links to inode in file system
  uint size;            // Size of file (bytes)
  uint addrs[NDIRECT+NINDIRECT_1+NINDIRECT_2+NINDIRECT_3];   // Data block addresses
};
```

struct dinode만 변경하는 것에서 그치면 안 되고, struct dinode의 복사본을 저장하고 있는 file.h의 struct inode의 addrs의 크기도 함께 수정해야 한다.

### 2-5. bmap 수정

bn번째 블록에 접근할 때 아직 블록이 할당되어 있지 않았다면 블록을 할당하고, bn번째 블록의 주소를 반환하는 bmap 함수도 수정해야 한다. 기본적인 동작 과정은 bread를 이용해 addrs 배열을 읽어들인 다음 lock하고, 접근하고자 하는 블록이 아직 할당되어 있지 않으면 balloc을 이용해 할당받아서 할당된 정보를 다시 log_write를 이용해 indirect block에 기록한 다음, 이 과정이 모두 끝나면 brelse를 이용해 unlock하는 것을 level 수 만큼 반복하는 것으로 구성되어 있다. 각 level에서 접근해야 하는 인덱스 값은 간단한 산수를 이용해 구할 수 있다.

### 2-6. iappend 수정

2-5까지만 작업하더라도 ssufs_test를 통과할 수 있지만, 파일 시스템을 온전히 수정하기 위해서는 xv6를 빌드할 때 파일 시스템 이미지를 만들 때 사용하는 mkfs.c의 iappend 함수도 수정해야 한다. 이 함수도 bmap과 유사한 방식으로 작업할 수 있으며, balloc 대신 xint(freeblock++)를 이용해 블록을 할당받는다는 것이 다르다.

### 2-7. itrunc 수정

파일 삭제를 올바르게 수행하기 위해서는 fs.c의 itrunc도 수정해야 한다. direct block, 그리고 indirect block이 가리키는 모든 블록을 보면서 할당되어 있는 모든 블록에 대해 bfree를 호출하면 된다.

# 3. 결과

생략

# 4. 소스 코드

### 4-1. Makefile

```makefile
UPROGS=\
  ...
  _ssualloc_test\
  _ssufs_test\

EXTRA=\
  ...
  ssualloc_test.c\
  ssufs_test.c\
```

### 4-2. file.h

```c
// in-memory copy of an inode
struct inode {
  ...
  uint addrs[NDIRECT+NINDIRECT_1+NINDIRECT_2+NINDIRECT_3];
};
```

### 4-3. fs.c

```c
static uint bmap(struct inode *ip, uint bn){
  uint addr, *a;
  struct buf *bp;
  // direct block
  if(bn < NDIRECT){
    if((addr = ip->addrs[bn]) == 0)
      ip->addrs[bn] = addr = balloc(ip->dev);
    return addr;
  }
  bn -= NDIRECT;
  int index_offset[4] = {
    0,
    NDIRECT,
    NDIRECT + NINDIRECT_1,
    NDIRECT + NINDIRECT_1 + NINDIRECT_2
  };
  int block_size[4] = {
    1,
    NINDIRECT,
    NINDIRECT * NINDIRECT,
    NINDIRECT * NINDIRECT * NINDIRECT
  };
  int mod = NINDIRECT;
  // 1-level indirect block
  if(bn < block_size[1] * NINDIRECT_1){
    // calculate index
    int idx1 = bn / block_size[1] % mod + index_offset[1]; // dinode index
    int idx2 = bn / block_size[0] % mod; // 1-level index
    // load indirect block, allocate if need
    if((addr = ip->addrs[idx1]) == 0)
      ip->addrs[idx1] = addr = balloc(ip->dev);
    // read & lock indirect block
    bp = bread(ip->dev, addr);
    a = (uint*)bp->data;
    // load block
    if((addr = a[idx2]) == 0){
      a[idx2] = addr = balloc(ip->dev);
      log_write(bp);
    }
    brelse(bp);
    return addr;
  }
  bn -= block_size[1] * NINDIRECT_1;
  // 2-level indirect block
  if(bn < NINDIRECT * NINDIRECT * NINDIRECT_2){
    // calculate index
    int idx1 = bn / block_size[2] % mod + index_offset[2]; // dinode index
    int idx2 = bn / block_size[1] % mod; // 1-level index
    int idx3 = bn / block_size[0] % mod; // 2-level index
    // load first indirect block
    if((addr = ip->addrs[idx1]) == 0)
      ip->addrs[idx1] = addr = balloc(ip->dev);
    // read & lock first indirect block
    bp = bread(ip->dev, addr);
    a = (uint*)bp->data;
    // load second indirect block
    if((addr = a[idx2]) == 0){
      a[idx2] = addr = balloc(ip->dev);
      log_write(bp);
    }
    brelse(bp);
    // read & lock second indirect block
    bp = bread(ip->dev, addr);
    a = (uint*)bp->data;
    // load block
    if((addr = a[idx3]) == 0){
      a[idx3] = addr = balloc(ip->dev);
      log_write(bp);
    }
    brelse(bp);
    return addr;
  }
  bn -= block_size[2] * NINDIRECT_2;
  // 3-level indirect block
  if(bn < block_size[3] * NINDIRECT_3){
    // calculate index
    int idx1 = bn / block_size[3] % mod + index_offset[3]; // dinode index
    int idx2 = bn / block_size[2] % mod; // 1-level index
    int idx3 = bn / block_size[1] % mod; // 2-level index
    int idx4 = bn / block_size[0] % mod; // 3-level index
    // load first indirect block
    if((addr = ip->addrs[idx1]) == 0)
      ip->addrs[idx1] = addr = balloc(ip->dev);
    // read & lock first indirect block
    bp = bread(ip->dev, addr);
    a = (uint*)bp->data;
    // load second indirect block
    if((addr = a[idx2]) == 0){
      a[idx2] = addr = balloc(ip->dev);
      log_write(bp);
    }
    brelse(bp);
    // read & lock second indirect block
    bp = bread(ip->dev, addr);
    a = (uint*)bp->data;
    // load third indirect block
    if((addr = a[idx3]) == 0){
      a[idx3] = addr = balloc(ip->dev);
      log_write(bp);
    }
    brelse(bp);
    // read & lock third indirect block
    bp = bread(ip->dev, addr);
    a = (uint*)bp->data;
    // load block
    if((addr = a[idx4]) == 0){
      a[idx4] = addr = balloc(ip->dev);
      log_write(bp);
    }
    brelse(bp);
    return addr;
  }
  panic("bmap: out of range");
}

static void
itrunc(struct inode *ip)
{  struct buf *bp, *bp2, *bp3;
  uint *a, *b, *c;
  for(int i = 0; i < NDIRECT; i++){
    if(ip->addrs[i]){
      bfree(ip->dev, ip->addrs[i]);
      ip->addrs[i] = 0;
    }
  }
  for(int _i=0; _i<NINDIRECT_1; _i++){
    int i = NDIRECT + _i;
    if(!ip->addrs[i]) continue;
    bp = bread(ip->dev, ip->addrs[i]);
    a = (uint*)bp->data;
    for(int j=0; j<NINDIRECT; j++){
      if(a[j]) bfree(ip->dev, a[j]), a[j] = 0;
    }
    brelse(bp);
    bfree(ip->dev, ip->addrs[i]);
    ip->addrs[i] = 0;
  }
  for(int _i=0; _i<NINDIRECT_2; _i++){
    int i = NDIRECT + NINDIRECT_1 + _i;
    if(!ip->addrs[i]) continue;
    bp = bread(ip->dev, ip->addrs[i]);
    a = (uint*)bp->data;
    for(int j=0; j<NINDIRECT; j++){
      if(!a[j]) continue;
      bp2 = bread(ip->dev, a[j]);
      b = (uint*)bp2->data;
      for(int k=0; k<NINDIRECT; k++){
        if(b[k]) bfree(ip->dev, b[k]), b[k] = 0;
      }
      brelse(bp2);
      bfree(ip->dev, a[j]); a[j] = 0;
    }
    brelse(bp);
    bfree(ip->dev, ip->addrs[i]);
    ip->addrs[i] = 0;
  }
  for(int _i=0; _i<NINDIRECT_3; _i++){
    int i = NDIRECT + NINDIRECT_1 + NINDIRECT_2 + _i;
    if(!ip->addrs[i]) continue;
    bp = bread(ip->dev, ip->addrs[i]);
    a = (uint*)bp->data;
    for(int j=0; j<NINDIRECT; j++){
      if(!a[j]) continue;
      bp2 = bread(ip->dev, a[j]);
      b = (uint*)bp2->data;
      for(int k=0; k<NINDIRECT; k++){
        if(!b[k]) continue;
	bp3 = bread(ip->dev, b[k]);
	c = (uint*)bp3->data;
	for(int s=0; s<NINDIRECT; s++){
          if(c[s]) bfree(ip->dev, c[s]), c[s] = 0;
	}
	brelse(bp3);
	bfree(ip->dev, b[k]); b[k] = 0;
      }
      brelse(bp2);
      bfree(ip->dev, a[j]); a[j] = 0;
    }
    brelse(bp);
    bfree(ip->dev, ip->addrs[i]);
    ip->addrs[i] = 0;
  }
  /*if(ip->addrs[NDIRECT]){
    bp = bread(ip->dev, ip->addrs[NDIRECT]);
    a = (uint*)bp->data;
    for(j = 0; j < NINDIRECT; j++){
      if(a[j])
        bfree(ip->dev, a[j]);
    }
    brelse(bp);
    bfree(ip->dev, ip->addrs[NDIRECT]);
    ip->addrs[NDIRECT] = 0;
  }*/
  ip->size = 0;
  iupdate(ip);
}
```

### 4-4. fs.h

```c
#define NINDIRECT (BSIZE / sizeof(uint))
#define NDIRECT 6
#define NINDIRECT_1 4
#define NINDIRECT_2 2
#define NINDIRECT_3 1
#define MAXFILE (NDIRECT + NINDIRECT * NINDIRECT_1 + NINDIRECT * NINDIRECT * NINDIRECT_2 + NINDIRECT * NINDIRECT * NINDIRECT * NINDIRECT_3)

// On-disk inode structure
struct dinode {
  short type;           // File type
  short major;          // Major device number (T_DEV only)
  short minor;          // Minor device number (T_DEV only)
  short nlink;          // Number of links to inode in file system
  uint size;            // Size of file (bytes)
  uint addrs[NDIRECT+NINDIRECT_1+NINDIRECT_2+NINDIRECT_3];   // Data block addresses
};
```

### 4-5. ide.c

```c
static void idestart(struct buf *b){
  if(b == 0)
    panic("idestart");
  if(b->blockno >= FSSIZE) cprintf("b->blockno: %d, FSSIZE: %d\n", b->blockno, FSSIZE),
    panic("incorrect blockno");
  ...
```

### 4-6. mkfs.c

```c
void iappend(uint inum, void *xp, int n){
  char *p = (char*)xp;
  uint fbn, off, n1;
  struct dinode din;
  char buf[BSIZE];
  uint indirect[NINDIRECT];
  uint x;
  int block_size[4] = {
    1,
    NINDIRECT,
    NINDIRECT * NINDIRECT,
    NINDIRECT * NINDIRECT * NINDIRECT
  }, mod = NINDIRECT;
  rinode(inum, &din);
  off = xint(din.size);
  // printf("append inum %d at off %d sz %d\n", inum, off, n);
  while(n > 0){
    fbn = off / BSIZE;
    assert(fbn < MAXFILE);
    if(fbn < NDIRECT){
      if(xint(din.addrs[fbn]) == 0){
        din.addrs[fbn] = xint(freeblock++);
      }
      x = xint(din.addrs[fbn]);
    }
    else if(fbn < NDIRECT + block_size[1] * NINDIRECT_1){
      int bn = fbn - NDIRECT;
      int idx1 = bn / block_size[1] % mod + NDIRECT;
      int idx2 = bn / block_size[0] % mod;
      // alloc first indirect block
      if(xint(din.addrs[idx1]) == 0){
        din.addrs[idx1] = xint(freeblock++);
      }
      uint first_block = xint(din.addrs[idx1]);
      // load first indirect block
      rsect(first_block, (char*)indirect);
      // alloc block
      if(indirect[idx2] == 0){
        indirect[idx2] = xint(freeblock++);
        // write to first block
        wsect(first_block, (char*)indirect);
      }
      // save block id
      x = xint(indirect[idx2]);
    }
    else if(fbn < NDIRECT + block_size[1] * NINDIRECT_1 + block_size[2] * NINDIRECT_2){
      int bn = fbn - NDIRECT;
      int idx1 = bn / block_size[2] % mod + NDIRECT + NINDIRECT_1;
      int idx2 = bn / block_size[1] % mod;
      int idx3 = bn / block_size[0] % mod;
      // alloc first indirect block
      if(xint(din.addrs[idx1]) == 0){
        din.addrs[idx1] = xint(freeblock++);
      }
      uint first_block = xint(din.addrs[idx1]);
      // load first indirect block
      rsect(first_block, (char*)indirect);
      // alloc second indirect block
      if(indirect[idx2] == 0){
        indirect[idx2] = xint(freeblock++);
        // write to first block
        wsect(first_block, (char*)indirect);
      }
      uint second_block = xint(indirect[idx2]);
      // load second indirect block
      rsect(second_block, (char*)indirect);
      // alloc block
      if(indirect[idx3] == 0){
        indirect[idx3] = xint(freeblock++);
        // write to second block
        wsect(second_block, (char*)indirect);
      }
      
      x = xint(indirect[idx3]);
    }
    else if(fbn < NDIRECT + block_size[1] * NINDIRECT_1 + block_size[2] * NINDIRECT_2 + block_size[3] * NINDIRECT_3){
      int bn = fbn - NDIRECT;
      int idx1 = bn / block_size[3] % mod + NDIRECT + NINDIRECT_1 + NINDIRECT_2;
      int idx2 = bn / block_size[2] % mod;
      int idx3 = bn / block_size[1] % mod;
      int idx4 = bn / block_size[0] % mod;
      // alloc first indirect block
      if(xint(din.addrs[idx1]) == 0){
        din.addrs[idx1] = xint(freeblock++);
      }
      uint first_block = xint(din.addrs[idx1]);
      // load first indirect block
      rsect(first_block, (char*)indirect);
      // alloc second indirect block
      if(indirect[idx2] == 0){
        indirect[idx2] = xint(freeblock++);
        // write to first block
        wsect(first_block, (char*)indirect);
      }
      uint second_block = xint(indirect[idx2]);
      // load second indirect block
      rsect(second_block, (char*)indirect);
      // alloc third indirect block
      if(indirect[idx3] == 0){
        indirect[idx3] = xint(freeblock++);
        // write to second block
        wsect(second_block, (char*)indirect);
      }
      uint third_block = xint(indirect[idx3]);
      // load third indirect block
      rsect(third_block, (char*)indirect);
      // alloc block
      if(indirect[idx4] == 0){
        indirect[idx4] = xint(freeblock++);
        // write to third block
        wsect(third_block, (char*)indirect);
      }
      x = xint(indirect[idx4]);
    }
    /*else {
      if(xint(din.addrs[NDIRECT]) == 0){
        din.addrs[NDIRECT] = xint(freeblock++);
      }
      rsect(xint(din.addrs[NDIRECT]), (char*)indirect);
      if(indirect[fbn - NDIRECT] == 0){
        indirect[fbn - NDIRECT] = xint(freeblock++);
        wsect(xint(din.addrs[NDIRECT]), (char*)indirect);
      }
      x = xint(indirect[fbn-NDIRECT]);
    }*/
    n1 = min(n, (fbn + 1) * BSIZE - off);
    rsect(x, buf);
    bcopy(p, buf + off - (fbn * BSIZE), n1);
    wsect(x, buf);
    n -= n1;
    off += n1;
    p += n1;
  }
  din.size = xint(off);
  winode(inum, &din);
}
```

### 4-7. param.h

```c
#define FSSIZE       2500000  // size of file system in blocks
```

### 4-8. proc.c

```c
static struct proc* allocproc(void){
  ...
found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  p->vp = 0; // init page count
```

### 4-9. proc.h

```c
struct proc {
  ...
  char name[16];               // Process name (debugging)
  int vp;                      // number of not allocated virtual page
};
```

### 4-10. syscall.c

```c
extern int sys_ssualloc(void);
extern int sys_getvp(void);
extern int sys_getpp(void);

[SYS_ssualloc] sys_ssualloc,
[SYS_getvp]   sys_getvp,
[SYS_getpp]   sys_getpp,
```

### 4-11. syscall.h

```c
#define SYS_ssualloc 22
#define SYS_getvp  23
#define SYS_getpp  24
```

### 4-12. sysproc.c

```c
int sys_ssualloc(void){
  int addr;
  int n;
  if(argint(0, &n) < 0) return -1;
  if(n < 0 || n % 4096 != 0) return -1;
  addr = myproc()->sz;
  myproc()->sz += n;
  myproc()->vp += n / 4096;
  return addr;
}
int sys_getvp(void){
  return myproc()->sz / PGSIZE;
}
int sys_getpp(void){
  return myproc()->sz / PGSIZE - myproc()->vp;
}
```

### 4-13. trap.c

```c
extern int mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm);

  case T_PGFLT:
    {
      // copy from allocuvm
      uint addr_fault = rcr2();
      uint page_fault = PGROUNDDOWN(addr_fault);
      char* alloc = kalloc();
      memset(alloc, 0, PGSIZE);
      myproc()->vp -= 1;
      mappages(myproc()->pgdir, (char*)page_fault, PGSIZE, V2P(alloc), PTE_W | PTE_U);
    }
    break;
```

### 4-14. user.h

```c
int ssualloc(int);
int getvp(void);
int getpp(void);
```

### 4-15. usys.S

```
SYSCALL(ssualloc)
SYSCALL(getvp)
SYSCALL(getpp)
```

### 4-16. vm.c

```c
// static
int mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm)
```

