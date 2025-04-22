#ifndef __PROJECT_CONFIG_H__
#define __PROJECT_CONFIG_H__

#define FRAMELEN 256
#define FRAMESHIFT 128
#define FFTLEN 256
#define FFTSHIFT 8

#define MDLNAME d16t1.2

#define HDSTR2(x) #x
#define HDSTR(x) HDSTR2(x)
#define MKHEADER(x, NAME, y) x##NAME##y

#define DEFHEADER(NAME) MKHEADER(mdl_, NAME, .h)
#define MDL_HEADER_NM DEFHEADER(MDLNAME)
#define MDL_HEADER_H HDSTR(MDL_HEADER_NM)

#define DEFCONF(NAME) MKHEADER(mdl_, NAME, _conf.h)
#define MDL_OP_CONF_NM DEFCONF(MDLNAME)
#define MDL_OP_CONF_H HDSTR(MDL_OP_CONF_NM)

#endif
