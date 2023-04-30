
#include "../Include.h"
#include "../connection/TCPConnection.h"

using namespace std;

void printCharSizeCPP() {
    cout << "Char size in c++: " << sizeof(char) << endl;
}


int main() {
    initPlatform();
    printCharSizeCPP();

    int tunnelFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (tunnelFd < 0)exitWithError("Could not create tunnel");
    Tunnel tunnel(tunnelFd);

}

