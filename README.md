----------------------------------------------------------------------------------
Synthetic Graph Toolbox
README
----------------------------------------------------------------------------------
Author: Anson Fong
Edited: April 28, 2024

Directory

$SGT/                                       - main directory for SGT
    config/                                     - configuration files
        gg.config
        TLM.config
    input/                                  - .json for GPCC appears here
        ....
    lib/                                    - header files
        GPCC/                                   - Guided C++ header files
            ....
        python/                                 - Mapper files
            ....
        TLM2/                                   - TLM2 header files
            ....
    src/                                    - source code
        gg.cpp                                  - graph generator
        writeGCPP.cpp                           - graph --> guided c++
        writePY.cpp                             - graph --> .json and image
        writeTLM1.cpp                           - graph --> TLM1 SysC
        writeTLM2.cpp                           - graph --> TLM2 SysC
    Makefile
    README                                  - (YOU ARE HERE)
    setup.csh                               - environment variable settings (csh)

----------------------------------------------------------------------------------
