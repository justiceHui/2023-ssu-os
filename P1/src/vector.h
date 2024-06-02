#include "types.h"
#include "stat.h"
#include "user.h"

// C++의 std::vector<int>와 유사한 형태의 동적 배열을 구현
// 실제로 저장된 원소의 개수 `cnt`와 할당되어 있는 공간의 크기 `capacity`를 관리
// push_back_int와 pop_back_int 모두 amortized O(1) 시간에 동작함
typedef struct _vector_int{
  int *ptr;
  int size, capacity;
} vector_int;

// 새로운 동적 배열 생성
vector_int* new_vector_int(){
  vector_int* res = (vector_int*)malloc(sizeof(vector_int));
  res->ptr = (int*)malloc(sizeof(int) * 4);
  res->size = 0;
  res->capacity = 4;
  return res;
}

// 동적 배열 할당 해제
void delete_vector_int(vector_int* vec){
  free(vec->ptr);
  free(vec);
}

// capacity 변경 후 재할당 & 복사
void _vector_int_reserve(vector_int *vec, int cap){
  vec->capacity = cap;
  int *new_ptr = (int*)malloc(sizeof(int) * vec->capacity);
  for(int i=0; i<vec->size; i++) new_ptr[i] = vec->ptr[i];
  free(vec->ptr);
  vec->ptr = new_ptr;
}

// 원소를 추가할 공간이 없는 경우 기존의 2배 크기로 확장
void _vector_int_double_up(vector_int *vec){
  if(vec->size == vec->capacity) _vector_int_reserve(vec, vec->capacity * 2);
}

// 원소의 개수가 할당받은 공간의 1/4 이하가 되면 기존 크기의 절반으로 축소
void _vector_int_double_down(vector_int *vec){
  if(vec->capacity > 4 && vec->size * 4 <= vec->capacity) _vector_int_reserve(vec, vec->capacity / 2);
}

// 맨 위에 원소 추가
// 시간 복잡도: amortized O(1)
void push_back_int(vector_int* vec, int v){
  _vector_int_double_up(vec);
  vec->ptr[vec->size++] = v;
}

// 맨 뒤에 있는 원소 제거
// 시간 복잡도: amortized O(1)
void pop_back_int(vector_int* vec){
  vec->size -= 1;
  _vector_int_double_down(vec);
}
