#include <iostream>
#include <fstream>

#define SZ 20

int main() {
  std::ofstream of;

  // open file in binary mode
  of.open("one.bin", std::ios::binary | std::ios::out);
  if (of.fail()) {
    std::cout << "Cannot create file" << std::endl;
    return 1;
  }

  // write 20 bytes in it
  char buf[SZ];
  for (int i = 0; i <= SZ; i++) {
    buf[i] = i;
  }

  of.write(buf, SZ);

  if (of.fail()) {
    std::cout << "Write failed" << std::endl;
    of.close();
    return 2;
  }

  of.close();
  return 0;
}
