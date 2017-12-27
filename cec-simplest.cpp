// Build command:
// g++-4.8 -std=gnu++0x -fPIC -g -Wall -march=armv6 -mfpu=vfp -mfloat-abi=hard -isystem /opt/vc/include/ -isystem /opt/vc/include/interface/vcos/pthreads/ -isystem /opt/vc/include/interface/vmcs_host/linux/ -I/usr/local/include -L /opt/vc/lib -lcec -lbcm_host -ldl cec-simplest.cpp -o cec-simplest 
//#CXXFLAGS=-I/usr/local/include
//#LINKFLAGS=-lcec -ldl
#include <libcec/cec.h>

// cecloader.h uses std::cout _without_ including iosfwd or iostream
// Furthermore is uses cout and not std::cout
#include <iostream>
using std::cout;
using std::endl;
#include <libcec/cecloader.h>

#include "bcm_host.h"
//#LINKFLAGS=-lbcm_host

#include <algorithm>  // for std::min

#include <termios.h>
#include <unistd.h>

/* sleep until a key is pressed and return value. echo = 0 disables key echo. */
int keypress(unsigned char echo)
{
  struct termios savedState, newState;
  int c;

  if (-1 == tcgetattr(STDIN_FILENO, &savedState))
  {
    return EOF;     /* error on tcgetattr */
  }

  newState = savedState;

  if ((echo = !echo)) /* yes i'm doing an assignment in an if clause */
  {
    echo = ECHO;    /* echo bit to disable echo */
  }

  /* disable canonical input and disable echo.  set minimal input to 1. */
  newState.c_lflag &= ~(echo | ICANON);
  newState.c_cc[VMIN] = 1;

  if (-1 == tcsetattr(STDIN_FILENO, TCSANOW, &newState))
  {
    return EOF;     /* error on tcsetattr */
  }

  c = getchar();      /* block (withot spinning) until we get a keypress */

  /* restore the saved state */
  if (-1 == tcsetattr(STDIN_FILENO, TCSANOW, &savedState))
  {
    return EOF;     /* error on tcsetattr */
  }

  return c;
}


int main(int argc, char* argv[])
{
    // Initialise the graphics pipeline for the raspberry pi. Yes, this is necessary.
    bcm_host_init();

    // Set up the CEC config
    CEC::ICECCallbacks        cec_callbacks;
    CEC::libcec_configuration cec_config;
    cec_config.Clear();
    cec_callbacks.Clear();

    const std::string devicename("CECExample");
    devicename.copy(cec_config.strDeviceName, std::min(devicename.size(), 13u) );
    
    cec_config.clientVersion       = CEC::LIBCEC_VERSION_CURRENT;
    cec_config.bActivateSource     = 0;
    cec_config.callbacks           = &cec_callbacks;
    cec_config.deviceTypes.Add(CEC::CEC_DEVICE_TYPE_RECORDING_DEVICE);

    // Get a cec adapter by initialising the cec library
    CEC::ICECAdapter* cec_adapter = LibCecInitialise(&cec_config);
    if( !cec_adapter )
    { 
        std::cerr << "Failed loading libcec.so\n"; 
        return 1; 
    }

    // Try to automatically determine the CEC devices 
    CEC::cec_adapter_descriptor devices[10];
    int8_t devices_found = cec_adapter->DetectAdapters(devices, 10, NULL, true);
    if( devices_found <= 0)
    {
        std::cerr << "Could not automatically determine the cec adapter devices\n";
        UnloadLibCec(cec_adapter);
        return 1;
    }

    // Open a connection to the zeroth CEC device
    if( !cec_adapter->Open(devices[0].strComName) )
    {        
        std::cerr << "Failed to open the CEC device on port " << devices[0].strComName << std::endl;
        UnloadLibCec(cec_adapter);
        return 1;
    }

    std::cout << "Ready to send commands..." << std::endl;

    CEC::cec_command play_cmd = cec_adapter->CommandFromString("14:44:44");
    CEC::cec_command stop_cmd = cec_adapter->CommandFromString("14:44:45");
    CEC::cec_command pause_cmd = cec_adapter->CommandFromString("14:44:46");
    bool playing = true;
    int value;
    while ((value = keypress(0)) != 'q')
    {
      if (value == ' ') // Space was pressed
      {
        cec_adapter->Transmit(playing ? pause_cmd : play_cmd);
        playing = !playing;
      }
      else if (value == '\x1B') // Escape was pressed
      {
        cec_adapter->Transmit(stop_cmd);
      }
    }
    
    // Close down and cleanup
    std::cout << "Exiting..." << std::endl;
    cec_adapter->Close();
    UnloadLibCec(cec_adapter);

    return 0;
}

