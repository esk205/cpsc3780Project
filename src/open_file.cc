#include <iostream>
#include <fstream>

#define SZ 20

int main() {
  std::ifstream ifi;

  // open file in binary mode
  ifi.open("one.bin", std::ios::binary);
  if (ifi.fail()) {
    std::cout << "Cannot open file" << std::endl;
    return 1;
  }

  // read 20 bytes in it
  char buf[SZ];

  if (!ifi.read(buf, SZ)) {
    std::cout << "Error reading" << std::endl;
    ifi.close();
    return 1;
  }
  else {
    std::cout << ifi.gcount() << " bytes read" << std::endl;
  }

  ifi.close();
  return 0;
}
