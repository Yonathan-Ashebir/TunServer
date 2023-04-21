
#include "Include.h"

using namespace std;

void printCharSizeCPP() {
    cout << "Char size in c++: " << sizeof(char) << endl;
}

int main() {
    initPlatform();
    printCharSizeCPP();
}
