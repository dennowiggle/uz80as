/* ===========================================================================
 * uz80as, an assembler for the Zilog Z80 and several other microprocessors.
 *
 * Hitachi HD 64180 CPU.
 * ===========================================================================
 */

#include "pp.h"
#include "err.h"
#include "options.h"
#include "uz80as.h"
#include <stddef.h>

/* pat:
 * 	a: expr
 * 	b: B,C,D,E,H,L,A
 * 	c: IX,IY (must be followed by + or -)
 * 	d: BC,DE,HL,SP
 * 	e: IX,IY
 * 	f: BC,DE,HL,AF
 * 	g: ADD,ADC,SUB,SBC,AND,XOR,OR,CP
 * 	h: INC,DEC
 * 	i: BC,DE,IX,SP
 * 	j: BC,DE,IY,SP
 * 	k: RLC,RRC,RL,RR,SLA,SRA,SRL
 * 	l: BIT,RES,SET
 * 	m: NZ,Z,NC,C,PO,PE,P,M
 * 	n: NZ,Z,NC,C
 * 	o: *
 *      p: B,C,D,E,IXH,IXL,A
 *      q: B,C,D,E,IYH,IYL,A
 *
 * gen:
 * 	.: output lastbyte
 * 	b: (op << 3) | lastbyte
 * 	c: op | lastbyte
 * 	d: lastbyte = op as 8 bit value
 * 	e: output op as word (no '.' sould follow)
 * 	f: (op << 4) | lastbyte
 * 	g: (op << 6) | lastbyte
 * 	h: *
 * 	i: relative jump to op
 * 	j: possible value to RST
 * 	k: possible value to IM
 * 	m: check arithmetic used with A register
 * 	n: check arithmetic used without A register
 */

static const struct matchtab s_matchtab_hd64180[] = {
	{ "LD b,b", "40b0c1." },
	{ "LD b,(HL)", "46b0." },
	{ "LD b,(ca)", "d1.46b0.d2." },
	{ "LD A,I", "ED.57." },
	{ "LD A,R", "ED.5F." },
	{ "LD A,(BC)", "0A." },
	{ "LD A,(DE)", "1A." },
	{ "LD A,(a)", "3A.e0" },
	{ "LD b,a", "06b0.d1." },
	{ "LD I,A", "ED.47." },
	{ "LD R,A", "ED.4F." },
	{ "LD SP,HL", "F9." },
	{ "LD SP,e", "d0.F9." },
	{ "LD HL,(a)", "2A.e0" },
	{ "LD d,(a)", "ED.4Bf0.e1" },
	{ "LD d,a", "01f0.e1" },
	{ "LD e,(a)", "d0.2A.e1" },
	{ "LD e,a", "d0.21.e1" },
	{ "LD (HL),b", "70c0." },
	{ "LD (HL),a", "36.d0." },
	{ "LD (BC),A", "02." },
	{ "LD (DE),A", "12." },
	{ "LD (ca),b", "d0.70c2.d1." },
	{ "LD (ca),a", "d0.36.d1.d2." },
	{ "LD (a),A", "32.e0" },
	{ "LD (a),HL", "22.e0" },
	{ "LD (a),d", "ED.43f1.e0" },
	{ "LD (a),e", "d1.22.e0" },
	{ "PUSH f", "C5f0." },
	{ "PUSH e", "d0.E5." },
	{ "POP f", "C1f0." },
	{ "POP e", "d0.E1." },
	{ "EX DE,HL", "EB." },
	{ "EX AF,AF'", "08." },
	{ "EX (SP),HL", "E3." },
	{ "EX (SP),e", "d0.E3." },
	{ "EXX", "D9." },
	{ "LDI", "ED.A0." },
	{ "LDIR", "ED.B0." },
	{ "LDD", "ED.A8." },
	{ "LDDR", "ED.B8." },
	{ "CPI", "ED.A1." },
	{ "CPIR", "ED.B1." },
	{ "CPD", "ED.A9." },
	{ "CPDR", "ED.B9." },
	{ "ADD HL,d", "09f0." },
	{ "ADD IX,i", "DD.09f0." },
	{ "ADD IY,j", "FD.09f0." },
	{ "ADC HL,d", "ED.4Af0." },
	{ "SBC HL,d", "ED.42f0." },
	{ "g A,b", "m080b0c1." },
	{ "g A,(HL)", "m086b0." },
	{ "g A,(ca)", "m0d1.86b0.d2." },
	{ "g A,a", "m0C6b0.d1." },
	{ "g b", "n080b0c1." },
	{ "g (HL)", "n086b0." },
	{ "g (ca)", "n0d1.86b0.d2." },
	{ "g a", "n0C6b0.d1." },
	{ "h b", "04b1c0." },
	{ "h (HL)", "34c0." },
	{ "h (ca)", "d1.34c0.d2." },
	{ "INC d", "03f0." },
	{ "INC e", "d0.23." },
	{ "DEC d", "0Bf0." },
	{ "DEC e", "d0.2B." },
	{ "DAA", "27." },
	{ "CPL", "2F." },
	{ "NEG", "ED.44." },
	{ "CCF", "3F." },
	{ "SCF", "37." },
	{ "NOP", "00." },
	{ "HALT", "76." },
	{ "DI", "F3." },
	{ "EI", "FB." },
	{ "IM a", "ED.k0." },
	{ "RLCA", "07." },
	{ "RLA", "17." },
	{ "RRCA", "0F." },
	{ "RRA", "1F." },
	{ "k b", "CB.00b0c1." },
	{ "k (HL)", "CB.06b0." },
	{ "k (ca)", "d1.CB.d2.06b0." },
	{ "RLD", "ED.6F." },
	{ "RRD", "ED.67." },
	{ "l a,b", "CB.00g0b1c2." },
	{ "l a,(HL)", "CB.06g0b1." },
	{ "l a,(ca)", "d2.CB.d3.06g0b1." },
	{ "JP (HL)", "E9." },
	{ "JP (e)", "d0.E9." },
	{ "JP m,a", "C2b0.e1" },
	{ "JP a", "C3.e0" },
	{ "JR n,a", "20b0.i1." },
	{ "JR a", "18.i0." },
	{ "DJNZ a", "10.i0." },
	{ "CALL m,a", "C4b0.e1" },
	{ "CALL a", "CD.e0" },
	{ "RETI", "ED.4D." },
	{ "RETN", "ED.45." },
	{ "RET m", "C0b0." },
	{ "RET", "C9." },
	{ "RST a", "C7j0." },
	{ "IN b,(C)", "ED.40b0." },
	{ "IN A,(a)", "DB.d0." },
	{ "IN F,(a)", "ED.70." },
	{ "INI", "ED.A2." },
	{ "INIR", "ED.B2." },
	{ "IND", "ED.AA." },
	{ "INDR", "ED.BA." },
	{ "OUT (C),b", "ED.41b0." },
	{ "OUT (a),A", "D3.d0." },
	{ "OUTI", "ED.A3." },
	{ "OTIR", "ED.B3." },
	{ "OUTD", "ED.AB." },
	{ "OTDR", "ED.BB." },
	/* */
	{ "IN0 b,(a)", "ED.00b0.d1." }, 
	{ "OUT0 (a),b", "ED.01b1.d0." },
	{ "OTDM", "ED.8B." },
	{ "OTDMR", "ED.9B." },
	{ "OTIM", "ED.83." },
	{ "OTIMR", "ED.93." },
	{ "MLT d", "ED.4Cf0." },
	{ "SLP", "ED.76." },
	{ "TST b", "ED.04b0." },
	{ "TST (HL)", "ED.34." },
	{ "TST a", "ED.64.d0." },
	{ "TSTIO a", "ED.74.d0." },
	{ NULL, NULL },
};

static const char *const bval[] = { "B", "C", "D", "E",
				    "H", "L", "", "A", NULL };
static const char *const cval[] = { "IX", "IY", NULL };
static const char *const dval[] = { "BC", "DE", "HL", "SP", NULL };
static const char *const fval[] = { "BC", "DE", "HL", "AF", NULL };
static const char *const gval[] = { "ADD", "ADC", "SUB", "SBC",
				    "AND", "XOR", "OR", "CP", NULL };
static const char *const hval[] = { "INC", "DEC", NULL };
static const char *const ival[] = { "BC", "DE", "IX", "SP", NULL };
static const char *const jval[] = { "BC", "DE", "IY", "SP", NULL };
static const char *const kval[] = { "RLC", "RRC", "RL", "RR",
				    "SLA", "SRA", "", "SRL", NULL };
static const char *const lval[] = { "", "BIT", "RES", "SET", NULL };
static const char *const mval[] = { "NZ", "Z", "NC", "C",
				    "PO", "PE", "P", "M", NULL };
static const char *const nval[] = { "NZ", "Z", "NC", "C", NULL };
static const char *const pval[] = { "B", "C", "D", "E",
				    "IXH", "IXL", "", "A", NULL };
static const char *const qval[] = { "B", "C", "D", "E",
				    "IYH", "IYL", "", "A", NULL };
static const char *const nullv[] = { NULL };

static const char *const *const valtab[] = { 
	bval, cval, dval, dval, fval,
       	gval, hval, ival, jval, kval,
       	lval, mval, nval, nullv, pval,
       	qval
};

static int indval(const char *p, int disp, const char **q)
{
	int v;
	const char *r;

	v = mreg(p, cval, &r);
	if (v >= 0) {
		v = (v == 0) ? 0xDD : 0xFD;
		if (!disp || *r == '+' || *r == '-') {
			*q = r;
			return v;
		}
	}
	return -1;
}

static int match_hd64180(char c, const char *p, const char **q)
{
	int v;

	if (c == 'c' || c == 'e') {
		v = indval(p, c == 'c', q);
	} else if (c <= 'q') {
		v = mreg(p, valtab[(int) (c - 'b')], q);
	} else {
		v = -1;
	}

	return v;
}

static int gen_hd64180(int *eb, char p, const int *vs, int i, int savepc)
{
	int b;
       
	b = *eb;
	switch (p) {
	case 'f': b |= (vs[i] << 4); break;
	case 'g': b |= (vs[i] << 6); break;
	case 'i': b = (vs[i] - savepc - 2); break;
	case 'j': if (s_pass > 0 && (vs[i] & ~56) != 0) {
			  eprint(_("invalid RST argument (%d)\n"),
				vs[i]);
			  eprcol(s_pline, s_pline_ep);
			  newerr();
		  }
		  b |= vs[i];
		  break;
	case 'k': if (s_pass > 0 && (vs[i] < 0 || vs[i] > 2)) {
			  eprint(_("invalid IM argument (%d)\n"),
				vs[i]);
			  eprcol(s_pline, s_pline_ep);
			  newerr();
		  }
		  b = 0x46;
		  if (vs[i] == 1)
			  b = 0x56;
		  else if (vs[i] == 2)
			  b = 0x5E;
		  break;
	case 'm': if (s_pass == 0 && !s_extended_op) {
			  if (vs[i] != 0 && vs[i] != 1 && vs[i] != 3) {
				eprint(_("unofficial syntax\n"));
				eprcol(s_pline, s_pline_ep);
				newerr();
			  }
		  }
		  break;
	case 'n': if (s_pass == 0 && !s_extended_op) {
			  if (vs[i] == 0 || vs[i] == 1 || vs[i] == 3) {
				eprint(_("unofficial syntax\n"));
				eprcol(s_pline, s_pline_ep);
				newerr();
			  }
		  }
		  break;
	default:
		  return -1;
	}

	*eb = b;
	return 0;
}

static int s_pat_char = 'b';
static int s_pat_index;

static void pat_char_rewind_hd64180(int c)
{
	s_pat_char = c;
	s_pat_index = 0;
};

static const char *pat_next_str_hd64180(void)
{
	const char *s;

	if (s_pat_char >= 'b' && s_pat_char <= 'q') {
		s = valtab[(int) (s_pat_char - 'b')][s_pat_index];
		if (s != NULL) {
			s_pat_index++;
		}
	} else {
		s = NULL;
	}

	return s;
};

const struct target s_target_hd64180 = {
	"hd64180",
	"Hitachi HD64180",
	s_matchtab_hd64180,
	match_hd64180,
	gen_hd64180,
	pat_char_rewind_hd64180,
	pat_next_str_hd64180
};

