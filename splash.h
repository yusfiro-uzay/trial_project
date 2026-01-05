#ifndef _splash_
#define _splash_

#ifdef __cplusplus
   extern "C" {
#endif


#undef PRINT_SPLASH

#define PRINT_SPLASH_OLD(info1,info2) printf("                                                                           \x1B[1;37m \n\
#\x1B[1;37m###################\x1B[1;37m################################\x1B[1;37m######################\x1B[1;37m#\n\
#\x1B[1;33m                   \x1B[1;36m      *                      *  \x1B[1;35m                      \x1B[1;37m#\n\
#\x1B[1;33m  *    dOOOOOOo.   \x1B[1;36m         ,O.       ,O.  ""%-7s"" \x1B[1;35m O OOOO      OO       \x1B[1;37m#\n\
#\x1B[1;33m     .`OOOO:' `OO. \x1B[1;36m     *  ,OOO.     ,OOO. ""%7s"" \x1B[1;35m O OOOO      OO     * \x1B[1;37m#\n\
#\x1B[1;33m     O.`OOOO.   YO \x1B[1;36m       .`OOOO.   .`OOOO.        \x1B[1;35m O OOOO      OO       \x1B[1;37m#\n\
#\x1B[1;33m     `O.`OOOO.     \x1B[1;36m      ,O.`OOOO. ,O.`OOOO.  *    \x1B[1;35m O OOOO      OO       \x1B[1;37m#\n\
#\x1B[1;33m   *  `O.`OOOO.  * \x1B[1;36m     ,O'O.`OOOO,O^O.`OOOO.      \x1B[1;35m O OOOO      OO       \x1B[1;37m#\n\
#\x1B[1;33m       `O.`OOOO.   \x1B[1;36m    ,O' `O.`OOOO' `O.`OOOO.     \x1B[1;35m O OOOO      OO  *    \x1B[1;37m#\n\
#\x1B[1;33m        `O.`OOOO.  \x1B[1;36m * ,O'   `O.`OO'   `O.`OOOO. *  \x1B[1;35m O OOOO      OO       \x1B[1;37m#\n\
#\x1B[1;33m    Ob   `O.`OOOO. \x1B[1;36m  ,O' *   `O.`'     `O.`OOOO.   \x1B[1;35m ` OOOO     ,OP       \x1B[1;37m#\n\
#\x1B[1;33m    `Ob.  ;O.`OOOO \x1B[1;36m ,O'       `O    *   `O.`OOOO.  \x1B[1;35m   OOOO   ,dOP     *  \x1B[1;37m#\n\
#\x1B[1;33m     `YOOOOP ,OOP' \x1B[1;36m,O'         `         `O.`OOOO. \x1B[1;35m *  `YOOOOOP'         \x1B[1;37m#\n\
#\x1B[1;33m             *     \x1B[1;36m       *                        \x1B[1;35m      *               \x1B[1;37m#\n\
#\x1B[1;37m###################\x1B[1;37m################################\x1B[1;37m######################\x1B[1;37m#\n\
\x1B[1;37m\n",info1,info2); fflush(0);

#define PRINT_SPLASH(info1,info2) printf("\
\x1B[1;37m\n\
                         \x1B[1;37m               o o       o\x1B[1;37m\n\
                         \x1B[1;37m          ###  # #  ##   #  ###  #   # #\x1B[1;37m\n\
           # # #      \x1B[1;37m\x1B[1;33m""%-12s""\x1B[1;37m  #   # #  # #  #   #   #   # #\x1B[1;37m\n\
        # # # # # #   \x1B[1;37m\x1B[1;33m""%-12s""\x1B[1;37m  #   # #  ###  #   #  ###  ##\x1B[1;37m\n\
      # #  ####  # #     \x1B[1;37m           #   # #  ##   #   #  # #  ##\x1B[1;37m\n\
     # # ######## # #    \x1B[1;37m           #   # #  # #  #   #  ###  # #\x1B[1;37m\n\
    # # ########## # #   \x1B[1;37m########## #   ###  ###  #   #  # #  # # #########\x1B[1;31m\n\
 @@@@@@######\x1B[1;37m#\x1B[1;31m#####@@@@@ ````````````   ```  ```  `   `  ` `  `````````````\x1B[1;31m\n\
  @@@@@@###\x1B[1;37m####\x1B[1;31m###@@@@@  ``````````````````````````````````````````````````\x1B[1;31m\n\
   @@@@@@#\x1B[1;37m######\x1B[1;31m#@@@@@    `###  ,###   `#######`      `#     `###     ###\x1B[1;31m\n\
    @@@@@\x1B[1;37m  ####  \x1B[1;31m@@@@     `###  ,###   `#######`     `###     `###   ###\x1B[1;31m\n\
     @@@@  \x1B[1;37m      \x1B[1;31m@@@      `###  ,###    ` ``###      `###      `### ###\x1B[1;31m\n\
      @@@@      @@@      \x1B[1;31m `###  ,###      `###      `## ##      `#####\x1B[1;31m\n\
       @@@@    @@@       \x1B[1;31m `###  ,###     `###       `## ##       `###\x1B[1;31m\n\
        @@@@  @@@        \x1B[1;31m `###  ,###    `###       ` #####       `###\x1B[1;31m\n\
         @@@@@@@         \x1B[1;31m `###  ,###   `###        `#######      `###\x1B[1;31m\n\
          @@@@@          \x1B[1;31m   #######    `#######   `###   ###     `###\x1B[1;31m\n\
           @@@           \x1B[1;31m    #####     `#######  `###`    ###    `###\x1B[1;31m\n\
            @\x1B[1;37m\n\
", info1, info2); fflush(0);



#define EGSE_PRINT_SPLASH()             PRINT_SPLASH("GSW","EGSE")
#define GRND_PRINT_SPLASH()             PRINT_SPLASH("GSW","GRND")
#define MBE_PRINT_SPLASH()              PRINT_SPLASH("GSW","MBE")
#define AUTH_PRINT_SPLASH()             PRINT_SPLASH("GSW","AUTH")

#define SMU_PRINT_SPLASH(info)          PRINT_SPLASH("FSW",info)

#ifdef __cplusplus
   }
#endif
#endif


