//
// Created by yoni_ash on 6/24/23.
//

#include <cstdio>
#include <iostream>


using namespace std;

void testNonLinearStruct() {

}

struct pseudo_header {
    unsigned int source_address;
    unsigned int dest_address;
    unsigned char placeholder;
    unsigned char protocol;
    unsigned short tcp_length;
};


unsigned int sum(const unsigned char *buf, unsigned size) {
    unsigned int sum = 0, i;

    /* Accumulate checksum */
    for (i = 0; i < size - 1; i += 2) {
        unsigned short word16 = *(unsigned short *) &buf[i];
        printf("word16: %d, i: %d, capacity: %d\n", word16, i, size);
//        cout << "word16: " << word16 << ", i: " << i << ", capacity:" << capacity << endl;
        sum += word16;
    }

    /* Handle an odd-sized case */
    if (size & 1) {
        unsigned short word16 = buf[i];
        printf("Other word16: %d, i: %d, capacity: %d\n", word16, i, size);
//        cout << "odd word16: " << word16 << ", i: " << i << ", capacity:" << capacity << endl;
        sum += word16;
    }


    printf("Sum ended\n");
//    cout << "Sum ended" << endl;
    return sum;
}


int sumPsh(pseudo_header &psh) {
//    printf("Ptr: %lx\n", (long) &psh);
    return sum((unsigned char *) &psh, sizeof(psh));
}

int main() {
    pseudo_header psh{1, 2, 3, 6, 4};
//    printf("tcpSumCheckTest %d, src: %lX, dest: %lX, place: %lX, proto %lX, tcpLen: %lX, sizeof: %lu\n",
//           sumPsh(psh),
//           (long) &psh.source_address, (long) &psh.dest_address, (long) &psh.placeholder, (long) &psh.protocol,
//           (long) &psh.tcp_length, sizeof(psh));
//    cout << "TcpSumCheck2: " << sumPsh(psh) << endl;
    printf("tcpSumCheckTest %d, src: %X, dest: %X, place: %d, proto %d, tcpLen: %d, sizeof: %lu\n",
           sumPsh(psh),
           psh.source_address, psh.dest_address, psh.placeholder, psh.protocol, psh.tcp_length, sizeof(psh));

}
