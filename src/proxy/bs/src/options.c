#include "options.h"
#include "log.h"
#include "readconf.h"
#include "build.h"
#include <getopt.h>
#include <unistd.h>
#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>

char *config_file = CONFIG_FILE_DEF;
int daemonize = 0;
int forcepfx = 0;

/** 
 * @brief 打印程序版本信息
 * 
 * @param[in] name 程序名称
 */
void print_bar( const char *name )
{
    char *mode = "";
    if( NULL != conf )
        mode = mode_frontend ? "frontend" : "backend";
    printf( "%s %s - version %s (compiled at %s %s)\n", \
            name, mode, VERSION, __DATE__, __TIME__ );
}

static void print_usage( char *name )
{
    const char *help_str[] = {
        "\noptions:\n",
        "   -f, --conf <file>   config file, default is ", CONFIG_FILE_DEF, "\n",
        "   -d, --daemon        make the server daemonize\n",
        "   -r, --forcepfx      force read pfx file from file\n",
        "   -v, --version       print version\n",
        "   -h, --help          print this help message\n",
        0
    };

    print_bar( name );
    int i = 0;
    for( i = 0; help_str[i]; i++ )
        printf( "%s", help_str[i] );
}

void print_version( const char *name )
{
    print_bar( name );
    print_buildinfo();
}

/** 
 * @brief 解析处理命令行选项
 * 
 * @param[in] argc
 * @param[in] argv[]
 */
void options_parse( int argc, char *argv[] )
{
    char *name = basename( argv[0] );

    struct option lopts[] = {
        { "config", required_argument, 0, 'f' },
        { "daemon", no_argument, 0, 'd' },
        { "forcepfx", no_argument, 0, 'r' },
        { "help", no_argument, 0, 'h' },
        { "version", no_argument, 0, 'v' },
        { 0, 0, 0, 0 }
    };

    int c = 0, opt_index = 0;
    while( ( c = getopt_long( argc, argv, "f:drhv", lopts, &opt_index ) ) != -1 )
    {
        switch( c )
        {
            case 'f':
                config_file = optarg;
                log_debug( "config file is %s", config_file );
                break;
            case 'd':
                daemonize = 1;
                log_debug( "set daemonize %d", daemonize );
                break;
            case 'r':
                forcepfx = 1;
                log_debug( "force read pfx from file" );
                break;
            case 'h':
                print_usage( name );
                exit( EXIT_SUCCESS );
                break;
            case 'v':
                print_version( name );
                exit( EXIT_SUCCESS );
                break;
            default:
                print_usage( name );
                exit( EXIT_FAILURE );
                break;
        }
    }
}
