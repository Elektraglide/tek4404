/*
       structure for UniFlex identifier returned by ufid call
*/

struct ufid { char     uf_ver;       /*  uniflex version number    */
              char     uf_rev;       /*  uniflex revision number   */
              char     uf_vendor;    /*  uniflex vendor number     */
              char     uf_inkey;     /*  uniflex install key byte  */
              unsigned uf_serno;     /*  uniflex serial number     */
              char     uf_spr[2];    /*  spare bytes               */
            } ;
