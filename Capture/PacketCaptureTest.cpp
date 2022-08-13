#include "PacketCapture.hpp"

int main(int argc, char** argv)
{
	PacketCapture Capture;
    if(argc < 2){
		printf("please input %s eth0\n", argv[0]);
		return -1;
	}
    bool ret = Capture.SetPromisc(argv[1], 1);
    ret = ret && Capture.BindDevice(argv[1]);
    ret = ret && Capture.InitRingBuffer(argv[1]);
    if(ret)
    {
        Capture.Run();
    }
	return 0;
}

// g++ PacketCaptureTest.cpp -o capture