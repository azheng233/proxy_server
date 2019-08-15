#include "license.h"
#include "lcsverify.h"
#include "options.h"
#include "readconf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static char devid[DEVID_LEN_MAX] = {0};
static int devid_len = 0;
//static time_t expiration[2] = {0};

#define LICENSE_BUF_LEN 4096

int license_verify( char *license_file )
{
#ifdef NO_LICENSE
    if( mode_frontend )
        devid_len = sprintf( devid, "BS-frontend$version-" VERSION "$902114200006" );
    else
        devid_len = sprintf( devid, "BS-backend$version-" VERSION "$902114200007" );
#else
    int result = 0;

    FILE *fp = fopen( license_file, "r" );
    if( NULL == fp )
    {
        perror( "open license file" );
        return -1;
    }

    unsigned char buf[LICENSE_BUF_LEN] = {0};
    int len = fread( buf, 1, LICENSE_BUF_LEN, fp );
    if( len <= 0 )
    {
        perror( "read license file" );
        fclose( fp );
        return -1;
    }
    fclose( fp );

    result = getDeviceNo( buf, devid, &devid_len );
    if( 0 != result )
    {
        fprintf( stderr, "getDeviceNo failed %d, len %d, %s\n", result, devid_len, devid );
        return result;
    }
    if( devid_len >= DEVID_LEN_MAX )
    {
        printf( "device id len %d too long\n", devid_len );
        devid_len = DEVID_LEN_MAX-1;
    }
    devid[devid_len] = 0;

    result = verifylicense( buf, len );
    if( 0 != result )
    {
        fprintf( stderr, "verify failed %d\n", result );
        return result;
    }

    char date[16] = {0};
    time_t t = time( NULL );
    strftime( date, 15, "%Y%m%d", localtime( &t ) );
    result = verifyDate( buf, date );
    if( 0 != result )
    {
        fprintf( stderr, "verifyDate failed %d, current date %s\n", result, date );
        return result;
    }

    char indate[32] = {0};
    //读取有效期
    result = getIndate( buf, indate );
    if( 0 != result )
    {
        fprintf( stderr, "getIndate failed %d, %s\n", result, indate );
        return result;
    }

    struct tm tm;
    int i;
    for( i=0; i<2; i++ )
    {
        memset( &tm, 0, sizeof(struct tm) );
        strptime( indate+8*i, "%Y%m%d", &tm );
        expiration[i] = mktime( &tm );
    }

#endif
    return 0;
}


int license_verify_date( time_t nowtime )
{
#ifndef NO_LICENSE
    if( nowtime < expiration[0] )
        return -1;
    if( nowtime > expiration[1] )
        return 1;
#endif
    return 0;
}

char *license_get_devid()
{
    return devid;
}
