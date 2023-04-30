
#include "../Include.h"
#include "../tunnel/DatagramTunnel.h"

using namespace std;

void printCharSizeCPP() {
    cout << "Char size in c++: " << sizeof(char) << endl;
}


int main() {
    initPlatform();
    printCharSizeCPP();

    int tunnelFd = socket(AF_INET, SOCK_DGRAM, 0);

    if (tunnelFd < 0)exitWithError("Could not create tunnel");
    DatagramTunnel tunnel(tunnelFd);

}

