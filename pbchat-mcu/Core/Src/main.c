#include "board.h"
#include "proto_link.h"

int main(void)
{
  LowLevel_Init();
  proto_link_init();

  while (1)
  {
    proto_link_poll();
  }
}
