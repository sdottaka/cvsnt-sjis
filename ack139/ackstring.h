/*	Copyright 1993 H.Ogasawara (COR.)	*/

/* v1.00  1993 10/10	Ogasawara Hiroyuki		*/
/*			oga@dgw.yz.yamagata-u.ac.jp	*/

extern unsigned char	__code_map[];
#define	Isdigit(code)	(__code_map[code]&1)
#define	IsSjis2(code)	(__code_map[code]&2)
#define	IsEuc(code)	(__code_map[code]&4)
#define	IsEuc2(code)	(__code_map[code]&4)
#define	IsCtrl(code)	(__code_map[code]&8)
#define	IsKana(code)	(__code_map[code]&16)

extern char	*SearchExtPosition();
extern int	*strcmpalp();

