

#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/unistd.h>
#include <signal.h>

#include <iostream>
#include <random>
#include <algorithm>


#include "fonts/MyriadWebPro_28.h"
#include "fonts/MyriadWebPro_Bold_28.h"
#include "fonts/MyriadWebPro_Bold_32.h"
#include "fonts/MyriadWebPro_Bold_46.h"
#include "fonts/MyriadWebPro_Bold_64.h"

#include "images/test_image_3.h"


#include "Bitmap.h"


using namespace std;


volatile bool keep_going = true;

void sigint_handler(int ) {
    keep_going = false;
}

int main(int argc, char** argv) {

    signal(SIGINT, sigint_handler);

    PoolImpl<254, 32> pool; // 5080 bytes.  pointers on MacOS X 10.8 are 8 bytes, so we need more space than on device.

    try {


        if (argc != 2) {
            printf("usage: %s <out filename>\n", argv[0]);
            exit(1);
        }

        Bitmap bmp(264, 176);
        Display& display = bmp;

        int h = bmp.height();
        int w = bmp.width();


        string hello = "Hello, world...";

        /// FONTS ////////////////////////////////////////////////////////////////////////////////////////////////////////
        Font myriadBold46(MyriadWebPro_Bold_46_pfo, MyriadWebPro_Bold_46_pfo + MyriadWebPro_Bold_46_pfo_size);
        Font myriadBold64(MyriadWebPro_Bold_64_pfo, MyriadWebPro_Bold_64_pfo + MyriadWebPro_Bold_64_pfo_size);
        Font myriad28(MyriadWebPro_28_pfo, MyriadWebPro_28_pfo + MyriadWebPro_28_pfo_size);
        Font myriadBold32(MyriadWebPro_Bold_32_pfo, MyriadWebPro_Bold_32_pfo + MyriadWebPro_Bold_32_pfo_size);
        Font myriadBold18(MyriadWebPro_Bold_28_pfo, MyriadWebPro_Bold_28_pfo + MyriadWebPro_Bold_28_pfo_size);

        #define NUM_FONTS 5
        Font* fonts[] = {&myriad28, &myriadBold46, &myriadBold64, &myriadBold32, &myriadBold18};

        rect meas[NUM_FONTS];
        std::transform (fonts, fonts+3, meas, [&](Font* f){
            return f->measureString(hello);
        });


        /// RANDOMNESS ////////////////////////////////////////////////////////////////////////////////////////////////////////



        std::default_random_engine random(23493459);
        std::uniform_int_distribution<uint8_t> fonti(0, NUM_FONTS-1);
        std::uniform_int_distribution<uint8_t> factori(5, 30);

        int i = 0;

        RLEImageMessageFactory factory(display.drawImage());
        factory.add(test_image_3, test_image_3_len);
        display.display();

        bmp.write_png_file(argv[1]);

        //delay(1000);

        exit(1);

        while(keep_going) {
            int32_t factor = factori(random);

            display.clear();
            if (i%2 == 0) display.fillCheckerboard(0, 0, w, h, i%4 == 0);

            volatile size_t fi = fonti(random);
            Font& f = *fonts[fi];
            rect& m = meas[fi];

            std::uniform_int_distribution<coord_t> randw(std::min(0, w-m.width), std::max(0, w-m.width));
            std::uniform_int_distribution<coord_t> randh(std::min(0, h-m.height), std::max(0,h-m.height));

            volatile coord_t xx = randw(random);
            volatile coord_t yy = randh(random);

            char buf[15];
            sprintf(buf, "%u", factor);
            string s(buf);
            rect sm = fonts[0]->measureString(s);

            display.drawString(xx, yy, hello, f,  i % 4 > 0);
            display.drawString(w-sm.width, h-sm.height, s, *fonts[0],  i % 4 > 0);
            display.display();
            i++;
            bmp.write_png_file(argv[1]);
        }
        bmp.write_png_file(argv[1]);
    } catch (...) {
        cerr << "Exception" << endl;
    }

    cerr << endl << pool.inUse() << " blocks in use at exit." << endl;


    return 0;
}