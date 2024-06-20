1 TMAX configuration File For TMAX System


lkrittseiap1d  TMAX_DIR = "/rextkr1/Anysp4/tmax",
                 APPDIR = "/rextkr1/ext/online",
                PATHDIR = "/rextkr1/tmax/path",
                TLOGDIR = "/rextkr1/tmax/log/tlog",
                ULOGDIR = "/rextkr1/tmax/log/ulog",
                SLOGDIR = "/rextkr1/tmax/log/slog",


##############################################################################
# SVRGROUP
##############################################################################
svg_amm         NODENAME = "lkrittseiap1d",
                APPDIR   = "/rextkr1/ext/online"
svg_tmax        NODENAME = "lkrittseiap1d",
                APPDIR   = "/rextkr1/ext/online"
svg_lstn        NODENAME = "lkrittseiap1d",
                APPDIR   = "/rextkr1/ext/online"
svg_gw          NODENAME = "lkrittseiap1d",
                APPDIR   = "/rextkr1/ext/online"

##############################################################################
# RQ SERVER GROUP Define
##############################################################################
u11_rqsvg1      NODENAME = lkrittseiap1d,
                SVGTYPE  = RQMGR,
                CPC      = 4

u11_rqsvg2      NODENAME = lkrittseiap1d,
                SVGTYPE  = RQMGR,
                CPC      = 4

u11_rqsvg3      NODENAME = lkrittseiap1d,
                SVGTYPE  = RQMGR,
                CPC      = 4

u11_rqsvg4      NODENAME = lkrittseiap1d,
                SVGTYPE  = RQMGR,
                CPC      = 4

u11_rqsvg5      NODENAME = lkrittseiap1d,
                SVGTYPE  = RQMGR,
                CPC      = 4

u11_rqsvg6      NODENAME = lkrittseiap1d,
                SVGTYPE  = RQMGR,
                CPC      = 4      

##############################################################################
# 시스템 SERVER GROUP Define
##############################################################################
u11_sysvg2      NODENAME = lkrittseiap1d,
                APPDIR   = "/rextkr1/ext/online"

u11_sysvg3      NODENAME = lkrittseiap1d,
                APPDIR   = "/rextkr1/ext/online"

u11_sysvg4      NODENAME = lkrittseiap1d,
                APPDIR   = "/rextkr1/ext/online"

u11_sysvg5      NODENAME = lkrittseiap1d,
                APPDIR   = "/rextkr1/ext/online" 

u11_sysvg6      NODENAME = lkrittseiap1d,
                APPDIR   = "/rextkr1/ext/online"

u11_sysvg7      NODENAME = lkrittseiap1d,
                APPDIR   = "/rextkr1/ext/online"

##############################################################################
# 공통 SERVER GROUP Define
##############################################################################
u11_exsvg2      NODENAME = lkrittseiap1d,
                APPDIR   = "/rextkr1/ext/online"