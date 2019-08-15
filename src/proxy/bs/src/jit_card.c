#include "jit_card.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CERT_LEN	(8192)

int get_jit_cert( unsigned char *buf, int *len )
{
    int rv;
    void *handle = NULL;
    char pin[] = "123456";
    char szErr[1024] = {0};
    char certdata[MAX_CERT_LEN] = {0};
    unsigned int nCertLen = MAX_CERT_LEN;

    rv = MAN_OpenDevice(&handle, pin);
    if (rv) {
        szErr[0] = 0;
        MAN_GetErrorString(rv, szErr);
        log_error( "MAN_OpenDevice failed %d:%s", rv, szErr );
        goto err;
    }
    log_trace( "OpenDevice success!" );

    rv = CRYPTO_GetCert(handle, 0, certdata, &nCertLen);
    if (rv) {
        szErr[0] = 0;
        MAN_GetErrorString(rv, szErr);
        log_error( "CRYPTO_GetCert(0) failed %d:%s", rv, szErr );
        goto err;
    }

    if( conf[LOG_LEVEL].value.num <= LOG_LEVEL_TRACE )
    {
        FILE *fp;
        fp=fopen("read.pfx","wb+");
        fwrite(certdata,1,nCertLen,fp);
        fclose(fp);
    }

    if(nCertLen>0)
    {    *len=nCertLen;
        memcpy(buf,certdata,nCertLen);
    }
    rv = RET_PAI_OK;

err:
    if (handle) {
        rv = MAN_CloseDevice(handle);
        if (rv) {
            szErr[0] = 0;
            MAN_GetErrorString(rv, szErr);
            log_error( "MAN_CloseDevice failed %d:%s", rv, szErr );
        }

        handle = NULL;
    }
    return rv;
}

