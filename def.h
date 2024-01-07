/*
    common
*/

#ifndef _def_H
#define _def_H

#define STRINGIFY(x)        #x
#define TOSTRING(x)         STRINGIFY(x)
#define PPCAT_NX(A, B)      A ## B

#define PTXT(s)             PSTR(TXT_ ## s)

#if !defined(FWVER_NUM1) && !defined(FWVER_NUM2) && !defined(FWVER_NUM3)
#define FWVER_NUM1          1
#define FWVER_NUM2          0
#define FWVER_NUM3          0
#endif

/* FW version name */
#if defined(FWVER_NUM1) && defined(FWVER_NUM2) && defined(FWVER_NUM3)

#define FWVER_NUM           FWVER_NUM1.FWVER_NUM2.FWVER_NUM3

#define FWVER_NAME          TOSTRING(FWVER_NUM)
#define FWVER_FILENAME      "alter-dev.v" FWVER_NAME

#define FWVER_DEBUG

#endif

#endif // _def_H
