#include "heartbeat.h"
#include "timehandle.h"
#include "log.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define HB_PATH_LEN  128

static const char *hb_name = "";
static char hb_pid_file[HB_PATH_LEN] = {0};
static char hb_heartbeat_file[HB_PATH_LEN] = {0};

void hb_init( const char *name, const char *path )
{
    if( name )
        hb_name = name;
    if( path )
    {
        snprintf( hb_pid_file, HB_PATH_LEN, "%s/%s.PID", path, hb_name );
        snprintf( hb_heartbeat_file, HB_PATH_LEN, "%s/%s_heartbeat.txt", path, hb_name );
    }
}

int hb_write_pid()
{
    FILE *fp = fopen( hb_pid_file, "w" );
    if( NULL==fp )
    {
        log_error( "open pid file %s failed, %s", hb_pid_file, strerror(errno) );
        return -1;
    }

    fprintf( fp, "[PID]\nPID=%d\n", getpid() );

    fclose( fp );
    return 0;
}

int hb_heartbeat()
{
    FILE *fp = fopen( hb_heartbeat_file, "w" );
    if( NULL==fp )
    {
        log_error( "open heartbeat file %s failed, %s", hb_heartbeat_file, strerror(errno) );
        return -1;
    }

    fprintf( fp, "[SERVICE_STATUS]\n%s=%lu\n", hb_name, curtime );

    fclose( fp );
    return 0;
}

