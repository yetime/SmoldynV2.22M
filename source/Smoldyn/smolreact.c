/* Steven Andrews, started 10/22/2001.
This is a library of functions for the Smoldyn program.  See documentation
called Smoldyn_doc1.pdf and Smoldyn_doc2.pdf.
Copyright 2003-2011 by Steven Andrews.  This work is distributed under the terms
of the Gnu General Public License (GPL). */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <float.h>
#include "math2.h"
#include "random2.h"
#include "Rn.h"
#include "rxnparam.h"
#include "smoldyn.h"
#include "Sphere.h"
#include "Zn.h"

#include "smoldyn_config.h"

#ifdef THREADING
#include <pthread.h>
#endif

#define CHECK(A) if(!(A)) goto failure; else (void)0
#define CHECKS(A,B) if(!(A)) {strncpy(erstr,B,STRCHAR-1);erstr[STRCHAR-1]='\0';goto failure;} else (void)0

/******************************************************************************/
/********************************** Reactions *********************************/
/******************************************************************************/

//????????? start of new code
void*
check_for_interbox_bireactions_threaded(void* data);
void*
check_for_intrabox_bireactions_threaded(void* data);
void*
unireact_threaded_calculate_reactions(void* data);

typedef struct PARAMSET_check_for_intrabox_bireations_Threaded {
	simptr sim;
	int ll_ndx_1;
	int first_ndx;
	int second_ndx;
	int ll_ndx_2;
	stack* output_stack;

} PARAMS_check_for_intrabox;

typedef struct PARAMSET_morebireact {
	rxnptr rxn_to_execute;
	moleculeptr mol_ptr_1;
	moleculeptr mol_ptr_2;
	int ll_ndx_1;
	int mol_ndx_1;
	int ll_ndx_2;
	enum EventType et;

} PARAMS_morebireact;

typedef struct PARAMSET_doreact {
	simptr sim;
	rxnptr rxn;
	moleculeptr mptr1;
	moleculeptr mptr2;
	int ll1;
	int m1;
	int ll2;
	int m2;
	double *pos;
	panelptr pnl;

} PARAMS_doreact;

typedef struct PARAMSET_unireact_threaded_calculate_reactions {
	simptr sim;
	int ll;
	int mol_ndx1;
	int mol_ndx2;
	stack* output_stack;

} PARAMS_unireact_threaded_calculate_reactions;
//??????????? end of new code block

/******************************************************************************/
/********************************* enumerated types ***************************/
/******************************************************************************/

/* rxnstring2rp.  Converts string to enumerated RevParam type.  This reads
either single letter inputs or full word inputs.  Unrecognized inputs are
returned as RPnone. */
enum RevParam rxnstring2rp(char *string) {
	enum RevParam ans;

	if (!strcmp(string, "i"))
		ans = RPirrev;
	else if (!strcmp(string, "a"))
		ans = RPconfspread;
	else if (!strcmp(string, "p"))
		ans = RPpgem;
	else if (!strcmp(string, "x"))
		ans = RPpgemmax;
	else if (!strcmp(string, "r"))
		ans = RPratio;
	else if (!strcmp(string, "b"))
		ans = RPunbindrad;
	else if (!strcmp(string, "q"))
		ans = RPpgem2;
	else if (!strcmp(string, "y"))
		ans = RPpgemmax2;
	else if (!strcmp(string, "s"))
		ans = RPratio2;
	else if (!strcmp(string, "o"))
		ans = RPoffset;
	else if (!strcmp(string, "f"))
		ans = RPfixed;
	else if (!strcmp(string, "irrev"))
		ans = RPirrev;
	else if (!strcmp(string, "confspread"))
		ans = RPconfspread;
	else if (!strcmp(string, "bounce"))
		ans = RPbounce;
	else if (!strcmp(string, "pgem"))
		ans = RPpgem;
	else if (!strcmp(string, "pgemmax"))
		ans = RPpgemmax;
	else if (!strcmp(string, "ratio"))
		ans = RPratio;
	else if (!strcmp(string, "unbindrad"))
		ans = RPunbindrad;
	else if (!strcmp(string, "pgem2"))
		ans = RPpgem2;
	else if (!strcmp(string, "pgemmax2"))
		ans = RPpgemmax2;
	else if (!strcmp(string, "ratio2"))
		ans = RPratio2;
	else if (!strcmp(string, "offset"))
		ans = RPoffset;
	else if (!strcmp(string, "fixed"))
		ans = RPfixed;
	else
		ans = RPnone;
	return ans;
}

/* rxnrp2string.  Converts RevParam enumerated type variable rp to a word in
string that can be displayed.  Unrecognized inputs, as well as RPnone, get
returned as "none". */
char *
rxnrp2string(enum RevParam rp, char *string) {
	if (rp == RPirrev)
		strcpy(string, "irrev");
	else if (rp == RPconfspread)
		strcpy(string, "confspread");
	else if (rp == RPbounce)
		strcpy(string, "bounce");
	else if (rp == RPpgem)
		strcpy(string, "pgem");
	else if (rp == RPpgemmax)
		strcpy(string, "pgemmax");
	else if (rp == RPpgemmaxw)
		strcpy(string, "pgemmaxw");
	else if (rp == RPratio)
		strcpy(string, "ratio");
	else if (rp == RPunbindrad)
		strcpy(string, "unbindrad");
	else if (rp == RPpgem2)
		strcpy(string, "pgem2");
	else if (rp == RPpgemmax2)
		strcpy(string, "pgemmax2");
	else if (rp == RPratio2)
		strcpy(string, "ratio2");
	else if (rp == RPoffset)
		strcpy(string, "offset");
	else if (rp == RPfixed)
		strcpy(string, "fixed");
	else
		strcpy(string, "none");
	return string;
}

/******************************************************************************/
/***************************** low level utilities ****************************/
/******************************************************************************/

/* readrxnname. */
int readrxnname(simptr sim, char *rname, int *orderptr, rxnptr *rxnpt) {
	int r, order;

	r = -1;
	for (order = 0; order < MAXORDER && r == -1; order++)
		if (sim->rxnss[order])
			r = stringfind(sim->rxnss[order]->rname, sim->rxnss[order]->totrxn,
					rname);
	order--;
	if (r >= 0) {
		if (orderptr)
			*orderptr = order;
		if (rxnpt)
			*rxnpt = sim->rxnss[order]->rxn[r];
	}
	return r;
}

/* rxnpackident */
int rxnpackident(int order, int maxspecies, int *ident) {
	if (order == 0)
		return 0;
	if (order == 1)
		return ident[0];
	if (order == 2)
		return ident[0] * maxspecies + ident[1];
	return 0;
}

/* rxnunpackident */
void rxnunpackident(int order, int maxspecies, int ipack, int *ident) {
	if (order == 0)
		;
	else if (order == 1)
		ident[0] = ipack;
	else if (order == 2) {
		ident[0] = ipack / maxspecies;
		ident[1] = ipack % maxspecies;
	}
	return;
}

/* rxnpackstate.  Packs of list of order molecule states that are listed in
mstate into a single value, which is returned. */
enum MolecState rxnpackstate(int order, enum MolecState *mstate) {
	if (order == 0)
		return 0;
	if (order == 1)
		return mstate[0];
	if (order == 2)
		return mstate[0] * MSMAX1 + mstate[1];
	return 0;
}

/* rxnunpackstate.  Unpacks a packed molecule state that is input in mspack
to order individual states in mstate. */
void rxnunpackstate(int order, enum MolecState mspack, enum MolecState *mstate) {
	if (order == 0)
		;
	else if (order == 1)
		mstate[0] = mspack;
	else if (order == 2) {
		mstate[0] = mspack / MSMAX1;
		mstate[1] = mspack % MSMAX1;
	}
	return;
}

/* rxnallstates.  Returns 1 if the listed reaction is permitted for all reactant
states and 0 if not. */
int rxnallstates(rxnptr rxn) {
	enum MolecState ms;
	int nms2o;

	if (rxn->rxnss->order == 0)
		return 0;
	nms2o = intpower(MSMAX1, rxn->rxnss->order);
	for (ms = 0; ms < nms2o && rxn->permit[ms]; ms++)
		;
	if (ms == nms2o)
		return 1;
	return 0;
}

/* rxnreactantstate.  Looks through the reaction permit element to see if the
reaction is permitted for any state or state combination.  If not, it returns 0;
if so, it returns 1.  Also, if the reaction is permitted and if mstate is not
NULL, the �simplest� permitted state is returned in mstate.  Preference is given
to MSsoln and MSbsoln, with other states investigated afterwards.  If convertb2f
is set to 1, any returned states of MSbsoln are converted to MSsoln before the
function returns.  This function always returns a singe states in mstate (or
MSnone if the reaction is not permitted at all), and never MSall or MSsome. */
int rxnreactantstate(rxnptr rxn, enum MolecState *mstate, int convertb2f) {
	int order, permit;
	enum MolecState ms, ms1, ms2;

	order = rxn->rxnss->order;
	permit = 0;

	if (order == 0)
		permit = 1;

	else if (order == 1) {
		if (rxn->permit[MSsoln]) {
			ms = MSsoln;
			permit = 1;
		}
		else if (rxn->permit[MSbsoln]) {
			ms = MSbsoln;
			permit = 1;
		}
		if (!permit) {
			for (ms = 0; ms < MSMAX1 && !rxn->permit[ms]; ms++)
				;
			if (ms < MSMAX1)
				permit = 1;
		}
		if (permit && convertb2f && ms == MSbsoln)
			ms = MSsoln;
		if (mstate) {
			if (permit)
				mstate[0] = ms;
			else
				mstate[0] = MSnone;
		}
	}

	else if (order == 2) {
		if (rxn->permit[MSsoln * MSMAX1 + MSsoln]) {
			ms1 = ms2 = MSsoln;
			permit = 1;
		}
		else if (rxn->permit[MSsoln * MSMAX1 + MSbsoln]) {
			ms1 = MSsoln;
			ms2 = MSbsoln;
			permit = 1;
		}
		else if (rxn->permit[MSbsoln * MSMAX1 + MSsoln]) {
			ms1 = MSbsoln;
			ms2 = MSsoln;
			permit = 1;
		}
		else if (rxn->permit[MSbsoln * MSMAX1 + MSbsoln]) {
			ms1 = MSbsoln;
			ms2 = MSbsoln;
			permit = 1;
		}
		if (!permit) {
			for (ms1 = 0; ms1 < MSMAX1 && !rxn->permit[ms1 * MSMAX1 + MSsoln]; ms1++)
				;
			if (ms1 < MSMAX1) {
				ms2 = MSsoln;
				permit = 1;
			}
		}
		if (!permit) {
			for (ms2 = 0; ms2 < MSMAX1 && !rxn->permit[MSsoln * MSMAX1 + ms2]; ms2++)
				;
			if (ms2 < MSMAX1) {
				ms1 = MSsoln;
				permit = 1;
			}
		}
		if (!permit) {
			for (ms = 0; ms < MSMAX1 * MSMAX1 && !rxn->permit[ms]; ms++)
				;
			if (ms < MSMAX1 * MSMAX1) {
				ms1 = ms / MSMAX1;
				ms2 = ms % MSMAX1;
				permit = 1;
			}
		}
		if (permit && convertb2f) {
			if (ms1 == MSbsoln)
				ms1 = MSsoln;
			if (ms2 == MSbsoln)
				ms2 = MSsoln;
		}
		if (mstate) {
			mstate[0] = permit ? ms1 : MSnone;
			mstate[1] = permit ? ms2 : MSnone;
		}
	}

	return permit;
}

/* findreverserxn.  Inputs the reaction defined by order order and reaction
number r and looks to see if there is a reverse reaction.  The molecule states
for the input reactants are not considered.  If there is a direct reverse
reaction, meaning the products of the input reaction (including states), are
themselves able to react to form the reactants of the input reaction (perhaps
with different states), then the function returns 1 and the order and reaction
number of the reverse reaction are pointed to by optr and rptr.  If there is no
direct reverse reaction, but the products of the input reaction are still able
to react, the function returns 2 and optr and rptr point to the first listed
continuation reaction.  The function returns 0 if the products do not react with
each other, if there are no reactants, or if there are no products.  -1 is
returned for illegal inputs.  Either or both of optr and rptr are allowed to be
sent in as NULL values if the respective pieces of output information are not of
interest. */
int findreverserxn(simptr sim, int order, int r, int *optr, int *rptr) {
	rxnssptr rxnss, rxnssr;
	rxnptr rxn, rxnr;
	int orderr, rr, rrreturn, rev, identr, identrprd, j, jr, work[MAXORDER];
	enum MolecState mstater, mstaterprd;

	if (!sim || order < 0 || order > MAXORDER || r < 0)
		return -1;
	rxnss = sim->rxnss[order];
	if (!rxnss || r >= rxnss->totrxn)
		return -1;
	rxn = rxnss->rxn[r];

	orderr = rrreturn = 0;
	if (order == 0 || rxn->nprod == 0 || rxn->nprod >= MAXORDER
			|| !sim->rxnss[rxn->nprod])
		rev = 0;
	else {
		orderr = rxn->nprod;
		rxnssr = sim->rxnss[orderr];
		identr = rxnpackident(orderr, rxnssr->maxspecies, rxn->prdident);
		mstater = rxnpackstate(orderr, rxn->prdstate);

		rev = 0;
		for (j = 0; j < rxnssr->nrxn[identr]; j++) {
			rr = rxnssr->table[identr][j];
			rxnr = rxnssr->rxn[rr];
			if (rxnr->permit[mstater]) {
				if (rev != 1 && rxnr->nprod == order && Zn_sameset(
						rxn->rctident, rxnr->prdident, work, order)) {
					identrprd = rxnpackident(order, rxnss->maxspecies,
							rxnr->prdident);
					mstaterprd = rxnpackstate(order, rxnr->prdstate);
					for (jr = 0; jr < rxnss->nrxn[identrprd]; jr++)
						if (rxnss->table[identrprd][jr] == r
								&& rxnss->rxn[r]->permit[mstaterprd]) {
							rev = 1;
							rrreturn = rr;
						}
				}
				if (!rev) {
					rev = 2;
					rrreturn = rr;
				}
			}
		}
	}

	if (optr)
		*optr = orderr;
	if (rptr)
		*rptr = rrreturn;
	return rev;
}

/* rxnisprod.  Determines if a molecule with identity i and state ms is the
product of any reaction, of any order, returning 1 if so and 0 if not.  If code
is 0, there are no additional conditions.  If code is 1, the molecule also has
to be displaced from the reaction position (i.e. either confspread or the
unbinding radius is non-zero) in order to qualify. */
int rxnisprod(simptr sim, int i, enum MolecState ms, int code) {
	int order, k, l,r, prd;
	rxnssptr rxnss;
	rxnptr rxn;

	for (order = 0; order < MAXORDER; order++) {
		rxnss = sim->rxnss[order];
		if (rxnss) {
			for (r = 0; r < rxnss->totrxn; r++) {
				rxn = rxnss->rxn[r];
				for (prd = 0; prd < rxn->nprod; prd++)
					if (rxn->prdident[prd] == i && rxn->prdstate[prd] == ms) {
						if (code == 0)
							return 1;
						//@Christine
						if (rxn->rparamt == RPconfspread)
							return 1;
						for (k = 0; k <= rxnss->sim->nrfs; k++) {
						  for(l = 0; l <=rxnss->sim->nrfs; l++) {
							if (rxn->unbindrad[k][l] && rxn->unbindrad[k][l] > 0)
								return 1;
							if (dotVVD(rxn->prdpos[k][l][prd], rxn->prdpos[k][l][prd], sim->dim)
								> 0)
							    return 1;
						  }
						}
						
					}
			}
		}
	}
	return 0;
}

/******************************************************************************/
/****************************** memory management *****************************/
/******************************************************************************/

/* rxnalloc.  Allocates and initializes a reaction structure of order order.
The reaction has order reactants, a permit element that is allocated, no
products, and most parameters are set to -1 to indicate that they have not been
set up yet. */
rxnptr rxnalloc(int order, int max_surface) {
	rxnptr rxn;
	int rct, nms2o, i;
	enum MolecState ms;

	CHECK(rxn=(rxnptr) malloc(sizeof(struct rxnstruct)))
;	rxn->rxnss=NULL;
	rxn->rname=NULL;
	rxn->rctident=NULL;
	rxn->rctstate=NULL;
	rxn->permit=NULL;
	rxn->nprod=0;
	rxn->prdident=NULL;
	rxn->prdstate=NULL;
	rxn->rate=-1;
	//rxn->bindrad2=-1; see below
	//rxn->prob=-1;
	//rxn->tau=-1;
	rxn->rparamt=RPnone;
	rxn->rparam=0;
	//rxn->unbindrad=-1;
	rxn->prdpos=NULL;
	rxn->cmpt=NULL;
	rxn->srf=NULL;


	if(order>0) {
		CHECK(rxn->rctident=(int*)calloc(order,sizeof(int)));
		for(rct=0;rct<order;rct++) rxn->rctident[rct]=0;
		CHECK(rxn->rctstate=(enum MolecState*)calloc(order,sizeof(int)));
		for(rct=0;rct<order;rct++) rxn->rctstate[rct]=MSnone;
		nms2o=intpower(MSMAX1,order);
		CHECK(rxn->permit=(int*)calloc(nms2o,sizeof(int)));
		for(ms=0;ms<nms2o;ms++) rxn->permit[ms]=0;}
	if(order==1 || order==2) {
		CHECK(rxn->bindrad2=(double**) calloc(max_surface+1, sizeof(double*)));
		for(rct=0;rct<max_surface+1;rct++) {
			rxn->bindrad2[rct]=NULL;
			CHECK(rxn->bindrad2[rct]=(double*) calloc(max_surface+1, sizeof(double)));
			for(i=0;i<max_surface+1;i++) rxn->bindrad2[rct][i]=-1;
		}
		CHECK(rxn->unbindrad=(double**) calloc(max_surface+1, sizeof(double*)));
		for(rct=0;rct<max_surface+1;rct++) {
		    rxn->unbindrad[rct]=NULL; 
			CHECK(rxn->unbindrad[rct]=(double*) calloc(max_surface+1, sizeof(double)));
			for(i=0;i<max_surface+1;i++) rxn->unbindrad[rct][i]=-1;
		}
		    
		CHECK(rxn->pgemptr=(double**) calloc(max_surface+1, sizeof(double*)));
		for(rct=0;rct<max_surface+1;rct++) {
			rxn->pgemptr[rct]=NULL;
			CHECK(rxn->pgemptr[rct]=(double*) calloc(max_surface+1, sizeof(double)));
			for(i=0;i<max_surface+1;i++) rxn->pgemptr[rct][i]=-1;
		}
		
		
		CHECK(rxn->prob=(double**) calloc(max_surface+1, sizeof(double*)));
		for(rct=0;rct<max_surface+1;rct++) {
			rxn->prob[rct]=NULL;
			CHECK(rxn->prob[rct]=(double*) calloc(max_surface+1, sizeof(double)));
			for(i=0;i<max_surface+1;i++) rxn->prob[rct][i]=-10;
		}
		
		CHECK(rxn->tau=(double**) calloc(max_surface+1, sizeof(double*)));
		for(rct=0;rct<max_surface+1;rct++) {
			rxn->tau[rct]=NULL;
			CHECK(rxn->tau[rct]=(double*) calloc(max_surface+1, sizeof(double)));
			for(i=0;i<max_surface+1;i++) rxn->tau[rct][i]=-1;
		}
		
	}	    

	return rxn;

	failure:
	rxnfree(rxn);
	return NULL;
  
}

/* rxnfree.  Frees a reaction structure. */
void rxnfree(rxnptr rxn) {
	int prd, i, ns, ns2;
	int nos = 0;

	if (!rxn)
		return;
	if (rxn->prdpos){
		for(ns=0;ns<rxn->rxnss->sim->nrfs+1;ns++){
		    for(ns2=0;ns2<rxn->rxnss->sim->nrfs+1; ns2++) {
			for (prd = 0; prd < rxn->nprod; prd++) {
			    free(rxn->prdpos[ns][ns2][prd]);
			}
			free(rxn->prdpos[ns][ns2]);
		    }
		    free(rxn->prdpos[ns]);
		}
	}
	free(rxn->prdpos);
	free(rxn->prdstate);
	free(rxn->prdident);
	free(rxn->permit);
	free(rxn->rctstate);
	free(rxn->rctident);

	if (rxn->rxnss->sim->nrfs)
		nos = rxn->rxnss->sim->nrfs+1;

	for (i = 0; i < nos; i++)
		if (rxn->bindrad2[i])
			free(rxn->bindrad2[i]);
	free(rxn->bindrad2);
	
	for (i = 0; i < nos; i++)
		if (rxn->unbindrad[i])
			free(rxn->unbindrad[i]);
	free(rxn->unbindrad);

	
	for (i = 0; i < nos; i++)
		if (rxn->pgemptr[i])
			free(rxn->pgemptr[i]);
	free(rxn->pgemptr);
	
	for (i = 0; i < nos; i++)
		if (rxn->tau[i])
			free(rxn->tau[i]);
	free(rxn->tau);
	
	for (i = 0; i < nos; i++)
		if (rxn->prob[i])
	free(rxn->prob[i]);
	free(rxn->prob);
	
	free(rxn);
	return;
}

/* rxnssalloc */
rxnssptr rxnssalloc(int order, int maxspecies) {
	rxnssptr rxnss;
	int ni2o, i;

	rxnss = NULL;
	CHECK(order>=0);
	CHECK(rxnss=(rxnssptr) malloc(sizeof(struct rxnsuperstruct)));
	rxnss->condition=SCinit;
	rxnss->sim=NULL;
	rxnss->order=order;
	rxnss->maxspecies=maxspecies;
	rxnss->maxlist=0;
	rxnss->nrxn=NULL;
	rxnss->table=NULL;
	rxnss->maxrxn=0;
	rxnss->totrxn=0;
	rxnss->rname=NULL;
	rxnss->rxn=NULL;
	rxnss->rxnmollist=NULL;

	if(order>0) {
		ni2o=intpower(maxspecies,order);
		CHECK(rxnss->nrxn=(int*) calloc(ni2o,sizeof(int)));
		for(i=0;i<ni2o;i++) rxnss->nrxn[i]=0;
		CHECK(rxnss->table=(int**) calloc(ni2o,sizeof(int*)));
		for(i=0;i<ni2o;i++) rxnss->table[i]=NULL;}
	return rxnss;

	failure:
	rxnssfree(rxnss);
	return NULL;
  
}

/* rxnssfree */
void rxnssfree(rxnssptr rxnss) {
	int r, i, ni2o;

	if (!rxnss)
		return;
	free(rxnss->rxnmollist);
	if (rxnss->rxn)
		for (r = 0; r < rxnss->maxrxn; r++)
			rxnfree(rxnss->rxn[r]);
	free(rxnss->rxn);
	if (rxnss->rname)
		for (r = 0; r < rxnss->maxrxn; r++)
			free(rxnss->rname[r]);
	free(rxnss->rname);
	if (rxnss->table) {
		ni2o = intpower(rxnss->maxspecies, rxnss->order);
		for (i = 0; i < ni2o; i++)
			free(rxnss->table[i]);
		free(rxnss->table);
	}
	free(rxnss->nrxn);
	free(rxnss);
	return;
}

/******************************************************************************/
/**************************** data structure output ***************************/
/******************************************************************************/

/* rxnoutput */
void rxnoutput(simptr sim, int order) {
	rxnssptr rxnss;
	int dim, maxlist, maxll2o, ll, ord, ni2o, i, j,k,l, r, rct, prd, rev,
			identlist[MAXORDER], orderr, rr, i1, i2, o2, r2;
	rxnptr rxn, rxnr;
	enum MolecState ms, ms1, ms2, nms2o, statelist[MAXORDER];
	double dsum, step, rate3, rparam, ratio, bindrad;
	char string[STRCHAR];
	enum RevParam rparamt;
	
	;

	printf("ORDER %i REACTION PARAMETERS\n", order);
	if (!sim || !sim->mols || !sim->rxnss[order]) {
		printf(" No reactions of order %i\n\n", order);
		return;
	}
	rxnss = sim->rxnss[order];
	if (rxnss->condition != SCok)
		printf(" structure condition: %s", simsc2string(rxnss->condition,
				string));
	dim = sim->dim;

	printf(" %i reactions defined, of %i allocated\n", rxnss->totrxn,
			rxnss->maxrxn);

	if (order > 0) {
		printf(" Reactive molecule lists:");
		if (!rxnss->rxnmollist)
			printf(" not set up yet");
		else {
			maxlist = sim->mols->maxlist;
			maxll2o = intpower(maxlist, order);
			for (ll = 0; ll < maxll2o; ll++)
				if (rxnss->rxnmollist[ll]) {
					printf(" ");
					for (ord = 0; ord < order; ord++)
						printf("%s%s", sim->mols->listname[(ll / intpower(
								maxlist, ord)) % maxlist],
								ord < order - 1 ? "+" : "");
				}
		}
		printf("\n");
	}

	if (order > 0) {
		printf(" Reactants, sorted by molecule species:\n");
		ni2o = intpower(rxnss->maxspecies, order);
		for (i = 0; i < ni2o; i++)
			if (rxnss->nrxn[i]) {
				rxnunpackident(order, rxnss->maxspecies, i, identlist);
				if (Zn_issort(identlist, order) >= 1) {
					printf("  ");
					for (ord = 0; ord < order; ord++)
						printf("%s%s", sim->mols->spname[identlist[ord]], ord
								< order - 1 ? "+" : "");
					printf(" :");
					for (j = 0; j < rxnss->nrxn[i]; j++)
						printf(" %s", rxnss->rname[rxnss->table[i][j]]);
					printf("\n");
				}
			}
	}

	printf(" Reaction details:\n");

	for (r = 0; r < rxnss->totrxn; r++) {
		rxn = rxnss->rxn[r];
		printf("  Reaction %s:", rxn->rname);
		if (order == 0)
			printf(" 0"); // reactants
		for (rct = 0; rct < order; rct++) {
			printf(" %s", sim->mols->spname[rxn->rctident[rct]]);
			if (rxn->rctstate[rct] != MSsoln)
				printf(" (%s)", molms2string(rxn->rctstate[rct], string));
			if (rct < order - 1)
				printf(" +");
		}
		printf(" ->");
		if (rxn->nprod == 0)
			printf(" 0"); // products
		for (prd = 0; prd < rxn->nprod; prd++) {
			printf(" %s", sim->mols->spname[rxn->prdident[prd]]);
			if (rxn->prdstate[prd] != MSsoln)
				printf(" (%s)", molms2string(rxn->prdstate[prd], string));
			if (prd < rxn->nprod - 1)
				printf(" +");
		}
		printf("\n");

		for (rct = 0; rct < order; rct++) // permit
			if (rxn->rctstate[rct] == MSsome)
				rct = order + 1;
		if (rct == order + 1) {
			printf("   permit:");
			nms2o = intpower(MSMAX1, order);
			for (ms = 0; ms < nms2o; ms++) {
				if (rxn->permit[ms]) {
					rxnunpackstate(order, ms, statelist);
					printf(" ");
					for (ord = 0; ord < order; ord++)
						printf("%s%s", molms2string(statelist[ms], string), ord
								< order - 1 ? "+" : "");
				}
			}
		}

		if (rxn->cmpt)
			printf("   compartment: %s\n", rxn->cmpt->cname);
		if (rxn->srf)
			printf("   surface: %s\n", rxn->srf->sname);
		
		for (i = 0; i <= rxnss->sim->nrfs; i++) {
		    for (j = 0; j <= rxnss->sim->nrfs; j++) {
		
			if(rxn->rate>=0) {
			    printf("   requested and actual rate constants: %g, %g\n",rxn->rate,rxncalcrate(sim,order,r,&(rxn->pgemptr[i][j]), i-1,j-1));
			}
			else {
			
			    printf("   actual rate constant: %g\n", rxncalcrate(sim, order, r,
					&(rxn->pgemptr[i][j]),i-1,j-1));
			}
		    }
		    if (rxn->pgemptr[i][j] >= 0){
				printf("   geminate recombination probability: %g\n", rxn->pgemptr[i][j]);
		    }
		}
		
			  
		for (i = 0; i <= rxnss->sim->nrfs; i++) {
			for (j = 0; j <= rxnss->sim->nrfs; j++) {
			    if (rxn->rparamt == RPconfspread)
				printf("   conformational spread reaction\n");
		
			    if (rxn->tau[i][j] >= 0)
				printf("   characteristic time: %g\n", rxn->tau[i][j]);
		
			    if (order == 0)
				printf("   average reactions per time step: %g\n", rxn->prob[i][j]);
			    else if (order == 1)
				printf("   reaction probability per time step: %g\n", rxn->prob[i][j]);
			    else if (rxn->prob[i][j] >= 0 && rxn->prob[i][j] != 1)
				printf("   reaction probability after collision: %g\n", rxn->prob[i][j]);

			    if (rxn->bindrad2[i][j] >= 0)
				printf("   binding radius: %g for surface %i x %i \n",
							sqrt(rxn->bindrad2[i][j]), (i - 1), (j - 1));
			}
		}
	

		if (rxn->nprod == 2) { 
			for (i = 0; i <= sim->nrfs; i++) {
			  for(j=0;j<=sim->nrfs;j++) {
				if (rxn->unbindrad[i][j] >= 0)
					printf("   unbinding radius: %g for surface %i x %i \n",
							rxn->unbindrad[i][j], i, j);
				else
					printf("   unbinding radius: 0\n");
			  }
			}
			if (rxn->rparamt != RPconfspread && rxn->rparamt != RPbounce
					&& findreverserxn(sim, order, r, &o2, &r2) == 1) {
				//rxnr = sim->rxnss[o2]->rxn[r2]; //Chr: doesn't really seem to be needed?
				//Chr
				for (i = -1; i < sim->nrfs; i++) {
					for (j = -1; j < sim->nrfs; j++) { 
						dsum = MolCalcDifcSum(sim, rxn->prdident[0],
								rxn->prdstate[0], rxn->prdident[1],
								rxn->prdstate[1], i, j);
						rate3 = rxncalcrate(sim, o2, r2, NULL, i, j);
						if (rxn->prdident[0] == rxn->prdident[1])
							rate3 *= 2; // same reactants
						rparamt = rxn->rparamt;
						rparam = rxn->rparam;
						if (rparamt == RPunbindrad)
							bindrad = bindingradius(rate3, 0, dsum, rparam, 0);
						else if (rparamt == RPratio)
							bindrad = bindingradius(rate3, 0, dsum, rparam, 1);
						else if (rparamt == RPpgem || rparamt == RPpgemmax
								|| rparamt == RPpgemmaxw) {
							bindrad = bindingradius(rate3 * (1.0 - rparam), 0,
									dsum, -1, 0);
							rxn->pgemptr[i+1][j+1] = rparam;
							
						}
						else   {
							bindrad = bindingradius(rate3, 0, dsum, -1, 0);
						}
						printf("   unbinding radius if dt were 0: %g\n",
								unbindingradius(rxn->pgemptr[i+1][j+1] , 0, dsum, bindrad));
					}
				}
			}
		}

		if (order == 2 && rxnreactantstate(rxn, statelist, 1)) {
			ms1 = statelist[0];
			ms2 = statelist[1];
			i1 = rxn->rctident[0];
			i2 = rxn->rctident[1];
			//Christine
			for (i = -1; i <sim->nrfs; i++) {
				for (j = -1; j < sim->nrfs; j++) {
					dsum = MolCalcDifcSum(sim, i1, ms1, i2, ms2, i, j); 
					rev = findreverserxn(sim, 2, r, &o2, &r2); 
					rate3 = rxncalcrate(sim, order, r, NULL, i, j); 
					if (i1 == i2)
						rate3 *= 2; // same reactants
					if (rev == 1) {
						rparamt = sim->rxnss[o2]->rxn[r2]->rparamt;
						rparam = sim->rxnss[o2]->rxn[r2]->rparam;
					}
					else {
						rparamt = RPnone;
						rparam = 0;
					}
					if (rparamt == RPconfspread)
						bindrad = -1;
					else if (rparamt == RPbounce)
						bindrad= -1;
					else if (rparamt == RPunbindrad)
						bindrad = bindingradius(rate3, 0, dsum, rparam, 0);
					else if (rparamt == RPratio)
						bindrad = bindingradius(rate3, 0, dsum, rparam, 1);
					else if (rparamt == RPpgem || rparamt == RPpgemmax
							|| rparamt == RPpgemmaxw)
						bindrad= bindingradius(rate3 * (1.0 - rparam), 0,
								dsum, -1, 0);
					else {
						bindrad = bindingradius(rate3, 0, dsum, -1, 0); 
					}
					
					if (bindrad>= 0)
						printf("   binding radius if dt were 0: %g\n", bindrad);
					
					step = sqrt(2.0 * dsum * sim->dt);
					ratio = step / sqrt(rxn->bindrad2[i+1][j+1]);
					printf("   mutual rms step length: %g\n", step);
					printf(
							"   step length / binding radius: %g (%s %s-limited)\n",
							ratio, ratio > 0.1 && ratio < 10 ? "somewhat"
									: "very", ratio > 1 ? "activation"
									: "diffusion");
					printf(
							"   effective activation limited reaction rate: %g\n",
							actrxnrate(step, sqrt(rxn->bindrad2[i+1][j+1]))
									/ sim->dt);
				}
			}
		}

		if (rxn->rparamt != RPbounce)
			if (findreverserxn(sim, order, r, &orderr, &rr) == 1)
				printf(
						"   with reverse reaction %s, equilibrium constant is: %g\n",
						sim->rxnss[orderr]->rname[rr], rxn->rate
								/ sim->rxnss[orderr]->rxn[rr]->rate);
		if (rxn->rparamt != RPnone)
			printf("   product placement method and parameter: %s %g\n",
					rxnrp2string(rxn->rparamt, string), rxn->rparam);
		for (prd = 0; prd < rxn->nprod; prd++) {
			for(i=0;i<=sim->nrfs;i++){
			  for(j=0;j<=sim->nrfs;j++){
			    if (dotVVD(rxn->prdpos[i][j][prd], rxn->prdpos[i][j][prd], dim) > 0) {
				printf("   product %s displacement: ",
						sim->mols->spname[rxn->prdident[prd]]);
				printVD(rxn->prdpos[i][j][prd], dim);
			    }
			  }
			}
		}

		// The following segment does not account for non-1 reaction probabilities.
		if (rxn->nprod == 2 && sim->rxnss[2] && rxn->rparamt != RPconfspread
				&& rxn->rparamt != RPbounce) {
			i = rxnpackident(2, rxnss->maxspecies, rxn->prdident);
			for (j = 0; j < sim->rxnss[2]->nrxn[i]; j++) {
				rr = sim->rxnss[2]->table[i][j];
				rxnr = sim->rxnss[2]->rxn[rr];
				for (k = -1; k < sim->nrfs; k++) {
					for (l = -1; l < sim->nrfs; l++) {
						if (rxnr->bindrad2[k + 1][l + 1] >= 0 && rxnr->rparamt
								!= RPconfspread) {
							dsum = MolCalcDifcSum(sim, rxn->prdident[0],
									rxn->prdstate[0], rxn->prdident[1],
									rxn->prdstate[1], k, l);
							step = sqrt(2.0 * sim->dt * dsum);
							rxn->pgemptr[k+1][l+1] = 1.0 - numrxnrate(step, sqrt(rxnr->bindrad2[k
									+ 1][l + 1]), -1) / numrxnrate(step, sqrt(
									rxnr->bindrad2[k + 1][l + 1]),
									rxn->unbindrad[k + 1][l + 1]);
							rev = (rxnr->nprod == order);
							rev = rev && Zn_sameset(rxnr->prdident,
									rxn->rctident, identlist, order);
							printf(
									"   probability of geminate %s reaction '%s' is %g\n",
									rev ? "reverse" : "continuation",
									rxnr->rname, rxn->pgemptr[k+1][l+1] );
						}
					}
				}
			}
		}
	}

	printf("\n");
	return;
}

/* writereactions.  Writes all information about all reactions to the file fptr
using a format that can be read by Smoldyn.  This allows a simulation state to
be saved. */
void writereactions(simptr sim, FILE *fptr) {
	int order, r, i, j, prd, d, rct;
	rxnptr rxn;
	rxnssptr rxnss;
	char string[STRCHAR], string2[STRCHAR];
	enum MolecState ms, ms1, ms2;
	enum RevParam rparamt;

	fprintf(fptr, "# Reaction parameters\n");
	for (order = 0; order <= 2; order++)
		if (sim->rxnss[order]) {
			rxnss = sim->rxnss[order];
			for (r = 0; r < rxnss->totrxn; r++) {
				rxn = rxnss->rxn[r];

				if (rxn->cmpt)
					fprintf(fptr, "reaction_cmpt %s", rxn->cmpt->cname);
				else if (rxn->srf)
					fprintf(fptr, "reaction_surface %s", rxn->srf->sname);
				else
					fprintf(fptr, "reaction");
				fprintf(fptr, " %s", rxn->rname);
				if (order == 0)
					fprintf(fptr, " 0"); // reactants
				for (rct = 0; rct < order; rct++) {
					fprintf(fptr, " %s", sim->mols->spname[rxn->rctident[rct]]);
					if (rxn->rctstate[rct] != MSsoln)
						fprintf(fptr, "(%s)", molms2string(rxn->rctstate[rct],
								string));
					if (rct < order - 1)
						fprintf(fptr, " +");
				}
				fprintf(fptr, " ->");
				if (rxn->nprod == 0)
					fprintf(fptr, " 0"); // products
				for (prd = 0; prd < rxn->nprod; prd++) {
					fprintf(fptr, " %s", sim->mols->spname[rxn->prdident[prd]]);
					if (rxn->prdstate[prd] != MSsoln)
						fprintf(fptr, "(%s)", molms2string(rxn->prdstate[prd],
								string));
					if (prd < rxn->nprod - 1)
						fprintf(fptr, " +");
				}
				if (rxn->rate >= 0)
					fprintf(fptr, " %g", rxn->rate);
				fprintf(fptr, "\n");

				
				for (i = 0; i <= sim->nrfs; i++) {
					for (j = 0; j <= sim->nrfs; j++) {
					    if (order == 1 && rxn->rctstate[0] == MSsome) {
					    for (ms = 0; ms < MSMAX; ms++)
						    if (rxn->permit[ms])
							fprintf(fptr, "reaction_permit %s %s\n",
									rxn->rname, molms2string(ms, string));
					    }
					    else if (order == 2 && (rxn->rctstate[0] == MSsome
						|| rxn->rctstate[1] == MSsome)) {
					      for (ms1 = 0; ms1 < MSMAX1; ms1++) {
						for (ms2 = 0; ms2 < MSMAX1; ms2++) {
						    if (rxn->permit[ms1 * MSMAX1 + ms2]) {
							fprintf(fptr, "reaction_permit %s %s %s\n",
										rxn->rname, molms2string(ms1, string),
										molms2string(ms2, string2));
				
						    }
						}
					      }
					    }

					    if (rxn->rparamt == RPconfspread)
						  fprintf(fptr, "confspread_radius %s %g\n",
									rxn->rname, rxn->bindrad2[i][j] < 0 ? 0
											: sqrt(rxn->bindrad2[i][j]));
					    if (rxn->rate < 0) {
						if (order == 0 && rxn->prob[i][j] >= 0)
						    fprintf(fptr, "reaction_production %s %g\n",
								rxn->rname, rxn->prob[i][j]);
						else if (order == 1 && rxn->prob[i][j] >= 0)
						    fprintf(fptr, "reaction_probability %s %g\n",
							      rxn->rname, rxn->prob[i][j]);
						
					
					    }
					    else if (order == 2) {
							if (rxn->bindrad2[i][j] >= 0) {
								fprintf(fptr, "binding_radius %s %g\n",
										rxn->rname, sqrt(rxn->bindrad2[i][j]));
							}
					    }
				
					    if ((order == 2 && rxn->prob[i][j] != 1 && rxn->rparamt
						!= RPconfspread) || (rxn->rparamt == RPconfspread
						&& rxn->rate < 0)){
					    fprintf(fptr, "reaction_probability %s %g\n", rxn->rname,
							rxn->prob[i][j]);
					    }
				    }
				}
  
				rparamt = rxn->rparamt;
				if (rparamt == RPirrev)
					fprintf(fptr, "product_placement %s irrev\n", rxn->rname);
				else if (rparamt == RPbounce || rparamt == RPpgem || rparamt
						== RPpgemmax || rparamt == RPratio || rparamt
						== RPunbindrad || rparamt == RPpgem2 || rparamt
						== RPpgemmax2 || rparamt == RPratio2)
					fprintf(fptr, "product_placement %s %s %g\n", rxn->rname,
							rxnrp2string(rparamt, string), rxn->rparam);
				else if (rparamt == RPoffset || rparamt == RPfixed) {
					for (prd = 0; prd < rxn->nprod; prd++) {
						fprintf(fptr, "product_placement %s %s %s\n",
								rxn->rname, rxnrp2string(rparamt, string),
								sim->mols->spname[rxn->prdident[prd]]);
						for (d = 0; d < sim->dim; d++){
						  for(i=0;i<sim->nrfs;i++){
						    for(j=0;j<sim->nrfs;j++){
							fprintf(fptr, " %g", rxn->prdpos[i][j][prd][d]);
						    }
						  }
						}
						fprintf(fptr, "\n");
					}
				}
			}
		}

	fprintf(fptr, "\n");
	return;
}

/* checkrxnparams.  Checks some parameters of reactions to make sure that they
are reasonable.  Prints warning messages to the display.  Returns the total
number of errors and, if warnptr is not NULL, the number of warnings in warnptr.
*/
int checkrxnparams(simptr sim, int *warnptr) {
	int d, dim, warn, error, i1, i2, j, nspecies, r, i, k, l, ct, j1, j2,
			order, prd;
	molssptr mols;
	double minboxsize, vol, amax, vol2, vol3;
	rxnptr rxn, rxn1, rxn2;
	rxnssptr rxnss;
	char **spname, string[STRCHAR], string2[STRCHAR];
	enum MolecState ms1, ms2, ms;

	error = warn = 0;
	dim = sim->dim;
	nspecies = sim->mols->nspecies;
	mols = sim->mols;
	spname = sim->mols->spname;

	for (order = 0; order <= 2; order++) { // condition
		rxnss = sim->rxnss[order];
		if (rxnss) {
			if (rxnss->condition != SCok) {
				warn++;
				printf(" WARNING: order %i reaction structure %s\n", order,
						simsc2string(rxnss->condition, string));
			}
		}
	}

	for (order = 0; order <= 2; order++) { // maxspecies
		rxnss = sim->rxnss[order];
		if (rxnss) {
			if (!sim->mols) {
				error++;
				printf(
						" SMOLDYN BUG: Reactions are declared, but not molecules\n");
			}
			else if (sim->mols->maxspecies != sim->rxnss[order]->maxspecies) {
				error++;
				printf(
						" SMOLDYN BUG: number of molecule species differ between mols and rxnss\n");
			}
		}
	}

	for (order = 1; order <= 2; order++) { // reversible parameters
		rxnss = sim->rxnss[order];
		if (rxnss)
			for (r = 0; r < rxnss->totrxn; r++) {
				rxn = rxnss->rxn[r];
				if (rxn->rparamt == RPpgemmaxw) {
					printf(
							" WARNING: unspecified product parameter for reversible reaction %s.  Defaults are used.\n",
							rxn->rname);
					warn++;
				}
			}
	}

	rxnss = sim->rxnss[2]; // check for multiple bimolecular reactions with same reactants
	if (rxnss) {
		for (i1 = 1; i1 < nspecies; i1++)
			for (i2 = 1; i2 <= i1; i2++) {
				i = i1 * rxnss->maxspecies + i2;
				for (j1 = 0; j1 < rxnss->nrxn[i]; j1++) {
					rxn1 = rxnss->rxn[rxnss->table[i][j1]];
					for (j2 = 0; j2 < j1; j2++) {
						rxn2 = rxnss->rxn[rxnss->table[i][j2]];
						if (rxnallstates(rxn1) && rxnallstates(rxn2)) {
							printf(
									" WARNING: multiply defined bimolecular reactions: %s(all) + %s(all)\n",
									spname[i1], spname[i2]);
							warn++;
						}
						else if (rxnallstates(rxn1)) {
							for (ms2 = 0; ms2 <= MSMAX1; ms2++) {
								ms = MSsoln * MSMAX1 + ms2;
								if (rxn2->permit[ms]) {
									printf(
											" WARNING: multiply defined bimolecular reactions: %s(all) + %s(%s)\n",
											spname[i1], spname[i2],
											molms2string(ms2, string2));
									warn++;
								}
							}
						}
						else if (rxnallstates(rxn2)) {
							for (ms1 = 0; ms1 <= MSMAX1; ms1++) {
								ms = ms1 * MSMAX1 + MSsoln;
								if (rxn1->permit[ms]) {
									printf(
											" WARNING: multiply defined bimolecular reactions: %s(%s) + %s(all)\n",
											spname[i1], molms2string(ms1,
													string), spname[i2]);
									warn++;
								}
							}
						}
						else {
							for (ms1 = 0; ms1 < MSMAX1; ms1++)
								for (ms2 = 0; ms2 <= ms1; ms2++) {
									ms = ms1 * MSMAX1 + ms2;
									if (rxn1->permit[ms] && rxn2->permit[ms]) {
										printf(
												" WARNING: multiply defined bimolecular reactions: %s(%s) + %s(%s)\n",
												spname[i1], molms2string(ms1,
														string), spname[i2],
												molms2string(ms2, string2));
										warn++;
									}
								}
						}
					}
				}
			}
	}

	rxnss = sim->rxnss[2]; // warn that difm ignored for reaction rates
	if (rxnss)
		for (i1 = 1; i1 < nspecies; i1++)
			if (mols->difm[i1][MSsoln])
				for (i2 = 1; i2 < i1; i2++)
					for (j = 0; j
							< rxnss->nrxn[i = i1 * rxnss->maxspecies + i2]; j++) {
						rxn = rxnss->rxn[rxnss->table[i][j]];
						if (rxn->rate) {
							printf(
									" WARNING: diffusion matrix for %s was ignored for calculating rate for reaction %s\n",
									spname[i1], rxn->rname);
							warn++;
						}
					}

	rxnss = sim->rxnss[2]; // warn that drift ignored for reaction rates
	if (rxnss)
		for (i1 = 1; i1 < nspecies; i1++)
			if (mols->drift[i1][MSsoln])
				for (i2 = 1; i2 < i1; i2++)
					for (j = 0; j
							< rxnss->nrxn[i = i1 * rxnss->maxspecies + i2]; j++) {
						rxn = rxnss->rxn[rxnss->table[i][j]];
						if (rxn->rate) {
							printf(
									" WARNING: drift vector for %s was ignored for calculating rate for reaction %s\n",
									spname[i1], rxn->rname);
							warn++;
						}
					}

	for (order = 1; order <= 2; order++) { // product surface-bound states imply reactant surface-bound
		rxnss = sim->rxnss[order];
		if (rxnss) {
			for (i = 1; i < intpower(rxnss->maxspecies, order); i++)
				for (j = 0; j < rxnss->nrxn[i]; j++) {
					rxn = rxnss->rxn[rxnss->table[i][j]];
					if (rxn->permit[order == 1 ? MSsoln : MSsoln * MSMAX1
							+ MSsoln]) {
						for (prd = 0; prd < rxn->nprod; prd++)
							if (rxn->prdstate[prd] != MSsoln) {
								printf(
										" ERROR: a product of reaction %s is surface-bound, but no reactant is\n",
										rxn->rname);
								error++;
							}
					}
				}
		}
	}

	for (order = 0; order <= 2; order++) { // reaction compartment
		rxnss = sim->rxnss[order];
		if (rxnss)
			for (r = 0; r < rxnss->totrxn; r++) {
				rxn = rxnss->rxn[r];
				if (rxn->cmpt) {
					if (order == 0 && rxn->cmpt->volume <= 0) {
						printf(
								" ERROR: reaction %s cannot work in compartment %s because the compartment has no volume\n",
								rxn->rname, rxn->cmpt->cname);
						error++;
					}
				}
			}
	}

	for (order = 0; order <= 2; order++) { // reaction surface
		rxnss = sim->rxnss[order];
		if (rxnss)
			for (r = 0; r < rxnss->totrxn; r++) {
				rxn = rxnss->rxn[r];
				if (rxn->srf) {
					if (order == 0 && surfacearea(rxn->srf, sim->dim, NULL)
							<= 0) {
						printf(
								" ERROR: reaction %s cannot work on surface %s because the surface has no area\n",
								rxn->rname, rxn->srf->sname);
						error++;
					}
				}
			}
	}

	rxnss = sim->rxnss[0]; // order 0 reactions
	if (rxnss)
		for (k = 0; k <= sim->nrfs; k++) {
		    for (l = 0; l <= sim->nrfs; l++) {
			for (r = 0; r < rxnss->totrxn; r++) {
			    rxn = rxnss->rxn[r];
			    if (rxn->prob[k][l] < 0) {
				printf(
						" WARNING: reaction rate not set for reaction order 0, name %s\n",
						rxn->rname);
				rxn->prob = 0;
				warn++;
			}
		}
				}
		}

	rxnss = sim->rxnss[1]; // order 1 reactions
	
	if (rxnss)
	    for (k = 0; k <= sim->nrfs; k++) {
	    for (l = 0; l <= sim->nrfs; l++) {
		for (r = 0; r < rxnss->totrxn; r++) {
			rxn = rxnss->rxn[r];
			if (rxn->prob[k][l] < 0) {
				printf(
						" WARNING: reaction rate not set for reaction order 1, name %s\n",
						rxn->rname);
				rxn->prob[k][l] = 0;
				warn++;
			}
			else if (rxn->prob[k][l] > 0 && rxn->prob[k][l] < 10.0 / (double) RAND2_MAX) {
				printf(
						" WARNING: order 1 reaction %s probability is at lower end of random number generator resolution\n",
						rxn->rname);
				warn++;
			}
			else if (rxn->prob[k][l] > ((double) RAND2_MAX - 10.0)
					/ (double) RAND2_MAX && rxn->prob[k][l] < 1.0) {
				printf(
						" WARNING: order 1 reaction %s probability is at upper end of random number generator resolution\n",
						rxn->rname);
				warn++;
			}
			else if (rxn->prob[k][l] > 0.2) {
				printf(
						" WARNING: order 1 reaction %s probability is quite high\n",
						rxn->rname);
				warn++;
			}
				    if (rxn->tau[k][l] < 5 * sim->dt) {
					printf(
						" WARNING: order 1 reaction %s time constant is only %g times longer than the simulation time step for surfaces %i x %i\n",
						rxn->rname, rxn->tau[k][l] / sim->dt, k,l);
					warn++;
				    }
				}
			}
		}
	minboxsize = sim->boxs->size[0];
	for (d = 1; d < dim; d++)
		if (sim->boxs->size[d] < minboxsize)
			minboxsize = sim->boxs->size[d];

	rxnss = sim->rxnss[2]; // order 2 reactions
	if (rxnss) {
		for (r = 0; r < rxnss->totrxn; r++) {
			rxn = rxnss->rxn[r];
			for (k = -1; k < sim->nrfs; k++) {
				for (l = -1; l < sim->nrfs; l++) {
					if (rxn->bindrad2[k + 1][l + 1] < 0) {
						if (rxn->rparamt == RPconfspread)
							printf(
									" WARNING: confspread radius not set for order 2 reaction %s and surface %i x %i\n",
									rxn->rname, k, l);
						else if (rxn->rparamt == RPbounce)
							printf(
									" WARNING: bounce radius not set for order 2 reaction %s and surface %i x %i\n",
									rxn->rname, k, l);
						else
							printf(
									" WARNING: reaction rate not set for reaction order 2, name %s and surface %i x %i\n",
									rxn->rname, k, l);
						rxn->bindrad2[k + 1][l + 1] = 0;
						warn++;
					}
					else if (sqrt(rxn->bindrad2[k + 1][l + 1]) > minboxsize) {
						if (rxn->rparamt == RPconfspread)
							printf(
									" ERROR: confspread radius for order 2 reaction %s and surface %i x %i is larger than box dimensions\n",
									rxn->rname, k, l);
						else if (rxn->rparamt == RPbounce)
							printf(
									" ERROR: bounce radius for order 2 reaction %s and surface %i x %i is larger than box dimensions\n",
									rxn->rname, k, l);
						else
							printf(
									" ERROR: binding radius for order 2 reaction %s and surface %i x %i is larger than box dimensions\n",
									rxn->rname, k, l);
						error++;
					}
					if (rxn->prob[k + 1][l + 1] < 0 || rxn->prob[k + 1][l + 1] > 1) {
						printf(
								" ERROR: reaction %s probability is not between 0 and 1\n",
								rxn->rname);
						error++;
					}
					else if (rxn->prob[k + 1][l + 1] < 1 && rxn->rparamt != RPconfspread
							&& rxn->rparamt != RPbounce) {
						printf(
								" WARNING: reaction %s probability is not accounted for in rate calculation\n",
								rxn->rname);
						warn++;
					}
					if (rxn->tau[k+1][l+1] < 5 * sim->dt) {
						printf(
								" WARNING: order 2 reaction %s time constant is only %g times longer than the simulation time step for surfaces %i x %i\n",
								rxn->rname, rxn->tau[k+1][l+1] / sim->dt, k+1, l+1);
						warn++;
					}
				}
			}
		}
	}

	rxnss = sim->rxnss[2]; // more order 2 reactions
	if (rxnss) {
		vol = systemvolume(sim);
		vol2 = 0;
		for (i = 1; i < nspecies; i++) {
			amax = 0;
			for (i1 = 1; i1 < nspecies; i1++)
				for (j = 0; j < rxnss->nrxn[i * rxnss->maxspecies + i1]; j++) {
					r = rxnss->table[i * rxnss->maxspecies + i1][j];
					rxn = rxnss->rxn[r];
					for (k = 0; k <= sim->nrfs; k++) {
						for (l = 0; l <= sim->nrfs; l++) {
							if (amax < sqrt(rxn->bindrad2[k][l]))
								amax = sqrt(rxn->bindrad2[k][l]);
						}
					}
				}
			ct = molcount(sim, i, MSsoln, NULL, -1);
			vol3 = ct * 4.0 / 3.0 * PI * amax * amax * amax;
			vol2 += vol3;
			if (vol3 > vol / 10.0) {
				printf(
						" WARNING: reactive volume of %s is %g %% of total volume\n",
						spname[i], vol3 / vol * 100);
				warn++;
			}
		}
		if (vol2 > vol / 10.0) {
			printf(
					" WARNING: total reactive volume is a large fraction of total volume\n");
			warn++;
		}
	}

	if (warnptr)
		*warnptr = warn;
	return error;
}

/******************************************************************************/
/*************************** parameter calculations ***************************/
/******************************************************************************/

/* rxnsetrate */
int rxnsetrate(simptr sim, int order, int r, char *erstr) {
	rxnssptr rxnss;
	int i, j, i1, i2, rev, o2, r2, permit, k,l;
	rxnptr rxn, rxn2;
	double vol, sum[MSMAX], sum2, rate3, dsum, rparam, unbindrad;
	enum MolecState ms, ms1, ms2, statelist[MAXORDER];
	enum RevParam rparamt;

	rxnss = sim->rxnss[order];
	rxn = rxnss->rxn[r];

	if (rxn->rparamt == RPconfspread) { // confspread
		if (rxn->rate < 0)
			return 1;
		if (rxn->nprod != order) {
			sprintf(erstr,
					"confspread reaction has a different number of reactants and products");
			return 3;
		}
		if (rxn->rate >= 0) {
			for(i=0;i<=sim->nrfs;i++){
			  for(j=0;j<=sim->nrfs;j++) {
			    rxn->prob[i][j] = 1.0 - exp(-sim->dt * rxn->rate);
	
			    
			  }
			}
		}
	}
	//Chr: Fucked up anyway?
	else if (order == 0) { // order 0
		if (rxn->rate < 0)
			return 1;
		if (rxn->cmpt)
			vol = rxn->cmpt->volume;
		else if (rxn->srf)
			vol = surfacearea(rxn->srf, sim->dim, NULL);
		else
			vol = systemvolume(sim);
		for(i=0;i<=sim->nrfs;i++){
			  for(j=0;j<=sim->nrfs;j++) {
			      rxn->prob[i][j] = rxn->rate * sim->dt * vol;
			  }
		}
	}

	else if (order == 1) { // order 1
		if (rxn->rate < 0)
			return 1;
		i = rxn->rctident[0];
		for (ms = 0; ms < MSMAX; ms++) {
			sum[ms] = 0;
			for (j = 0; j < rxnss->nrxn[i]; j++) {
				rxn2 = rxnss->rxn[rxnss->table[i][j]];
				if (rxn2->permit[ms] && rxn2->rate > 0)
					sum[ms] += rxn2->rate;
			}
		}

		for(k=0;k<=sim->nrfs;k++){
			  for(l=0;l<=sim->nrfs;l++) {
			    if (rxn->rate == 0)
			      rxn->prob[k][l] = 0;
			    else {
			      sum2 = 0;
				  for (ms = 0; ms < MSMAX; ms++) {
				    if (rxn->permit[ms] && sum2 == 0)
					sum2 = sum[ms];
				    else if (rxn->permit[ms] && sum2 != sum[ms]) {
					sprintf(
							erstr,
							"cannot assign reaction probability because different values are needed for different states");
					return 2;
				    }
				  }
			if (sum2 > 0)
				rxn->prob[k][l] = rxn->rate / sum2 * (1.0 - exp(-sim->dt * sum2)); // desired probability
			sum2 = 0;
			for (j = 0; j < rxnss->nrxn[i]; j++) {
				rxn2 = rxnss->rxn[rxnss->table[i][j]];
				if (rxn2 == rxn)
					j = rxnss->nrxn[i];
				else
					sum2 += rxn2->prob[k][l];
			}
			rxn->prob[k][l] = rxn->prob[k][l] / (1.0 - sum2);
			    }
			  }
		} // probability, accounting for prior reactions

		rev = findreverserxn(sim, 1, r, &o2, &r2);
		if (rev > 0 && o2 == 2) { 
			if (rxn->rparamt == RPnone) {
				rxn->rparamt = RPpgemmaxw;
				rxn->rparam = 0.2;
			}
		}
	}

	else if (order == 2) { // order 2
	    for(i=-1;i<sim->nrfs;i++){
	      for(j=-1;j<sim->nrfs;j++) {
		if (rxn->rate < 0) {
			if (rxn->prob[i+1][j+1] < 0)
				rxn->prob[i+1][j+1] = 1;
			return 1;
		}
		i1 = rxn->rctident[0];
		i2 = rxn->rctident[1];

		permit = rxnreactantstate(rxn, statelist, 1);
		ms1 = statelist[0];
		ms2 = statelist[1];

		rate3 = rxn->rate;
		if (i1 == i2)
			rate3 *= 2; // same reactants
		if ((ms1 == MSsoln && ms2 != MSsoln)
				|| (ms1 != MSsoln && ms2 == MSsoln))
			rate3 *= 2; // one surface, one solution

		rev = findreverserxn(sim, 2, r, &o2, &r2);
		if (rev == 1) {
			if (sim->rxnss[o2]->rxn[r2]->rparamt == RPnone) {
				sim->rxnss[o2]->rxn[r2]->rparamt = RPpgemmaxw;
				sim->rxnss[o2]->rxn[r2]->rparam = 0.2;
			}
			rparamt = sim->rxnss[o2]->rxn[r2]->rparamt;
			rparam = sim->rxnss[o2]->rxn[r2]->rparam;
		}
		else {
			rparamt = RPnone;
			rparam = 0;
		}
		//int count=0; 
				dsum = MolCalcDifcSum(sim, i1, ms1, i2, ms2, i, j); //for i,j=-1 default (no surface) 
				if (rxn->prob[i+1][j+1] < 0)
					rxn->prob[i+1][j+1] = 1;
				if (!permit)
					rxn->bindrad2[i + 1][j + 1] = 0;
				else if (rate3 <= 0)
					rxn->bindrad2[i + 1][j + 1] = 0;
				else if (dsum <= 0) {
					sprintf(erstr, "Both diffusion coefficients are 0");
					return 4;
					//count=count+=1;
				}
				else if (rparamt == RPunbindrad)
					rxn->bindrad2[i + 1][j + 1] = bindingradius(rate3, sim->dt,
							dsum, rparam, 0);
				else if (rparamt == RPbounce)
					rxn->bindrad2[i + 1][j + 1] = bindingradius(rate3, sim->dt,
							dsum, rparam, 0);
				else if (rparamt == RPratio)
					rxn->bindrad2[i + 1][j + 1] = bindingradius(rate3, sim->dt,
							dsum, rparam, 1);
				else if (rparamt == RPpgem)
					rxn->bindrad2[i + 1][j + 1] = bindingradius(rate3 * (1.0
							- rparam), sim->dt, dsum, -1, 0);
				else if (rparamt == RPpgemmax || rparamt == RPpgemmaxw) {
					rxn->bindrad2[i + 1][j + 1] = bindingradius(rate3, sim->dt,
							dsum, 0, 0);
					unbindrad = unbindingradius(rparam, sim->dt, dsum,
							rxn->bindrad2[i + 1][j + 1]);
					if (unbindrad > 0)
						rxn->bindrad2[i + 1][j + 1] = bindingradius(rate3
								* (1.0 - rparam), sim->dt, dsum, -1, 0);
				}
				else
					rxn->bindrad2[i + 1][j + 1] = bindingradius(rate3, sim->dt,
							dsum, -1, 0);

				rxn->bindrad2[i + 1][j + 1] *= rxn->bindrad2[i + 1][j + 1]; 
			}
		}
		
		//if(count==(rxnss->sim->nrfs*rxnss->sim->nrfs)) {
		//   sprintf(erstr, "All diffusion coefficient combinations are 0");
		//			return 4;
		//}
	}

	return 0;
}

/* rxnsetrates */
int rxnsetrates(simptr sim, int order, char *erstr) {
	rxnssptr rxnss;
	int r, er;

	if (!sim || !erstr || order < 0 || order > MAXORDER) {
		sprintf(erstr, "illegal inputs to rxnsetrates function");
		return 0;
	}
	rxnss = sim->rxnss[order];
	if (!rxnss)
		return -1;

	for (r = 0; r < rxnss->totrxn; r++) {
		er = rxnsetrate(sim, order, r, erstr);
		if (er > 1)
			return r;
	}

	return -1;
}

/* rxnsetproduct */
int rxnsetproduct(simptr sim, int order, int r, char *erstr) {
	rxnssptr rxnss;
	rxnptr rxn, rxnr;
	int er, nprod, orderr, rr, rev, i, j;
	double rpar, dc1, dc2, dsum, bindradr;
	enum RevParam rparamt;
	enum MolecState ms1, ms2;

	rxnss = sim->rxnss[order];
	rxn = rxnss->rxn[r];
	nprod = rxn->nprod;
	rpar = rxn->rparam;
	rparamt = rxn->rparamt;
	er = 0;

	if (nprod == 0) {
		if (!(rparamt == RPnone || rparamt == RPirrev)) {
			sprintf(erstr,
					"Illegal product parameter because reaction has no products");
			er = 1;
		}
	}

	else if (nprod == 1) {
		if (!(rparamt == RPnone || rparamt == RPirrev || rparamt == RPbounce
				|| rparamt == RPconfspread || rparamt == RPoffset || rparamt
				== RPfixed)) {
			sprintf(erstr,
					"Illegal product parameter because reaction only has one product");
			er = 2;
		}
	}

	else if (nprod == 2) {
		ms1 = rxn->prdstate[0];
		ms2 = rxn->prdstate[1];
		if (ms1 == MSbsoln)
			ms1 = MSsoln;
		if (ms2 == MSbsoln)
			ms2 = MSsoln;
		rev = findreverserxn(sim, order, r, &orderr, &rr);

		//Christine: Has to be done for differnet surfaces as well
		for (i = -1; i < rxnss->sim->nrfs; i++) {
			for (j = -1; j < rxnss->sim->nrfs; j++) {
				rpar = rxn->rparam;
				rparamt = rxn->rparamt;
				dc1 = MolCalcDifcSum(sim, rxn->prdident[0], ms1, 0, 0, i, j);
				dc2 = MolCalcDifcSum(sim, rxn->prdident[1], ms2, 0, 0, i, j);
				dsum = dc1 + dc2;

				if (rev == 0) {
					if (rparamt == RPpgem || rparamt == RPpgemmax || rparamt
							== RPpgemmaxw || rparamt == RPratio || rparamt
							== RPpgem2 || rparamt == RPpgemmax2 || rparamt
							== RPratio2) {
						sprintf(erstr,
								"Illegal product parameter because products don't react");
						er = 3;
					}
					else if (rparamt == RPunbindrad) {
						if (dsum == 0)
							dsum = (dc1 = 1.0) + (dc2 = 1.0);
						rxn->unbindrad[i + 1][j + 1] = rpar;
						rxn->prdpos[i+1][j+1][0][0] = rpar * dc1 / dsum;
						rxn->prdpos[i+1][j+1][1][0] = -rpar * dc2 / dsum;
					}
					else if (rparamt == RPbounce) {
						rxn->unbindrad[i + 1][j + 1] = rpar;
						rxn->prdpos[i+1][j+1][0][0] = 0;
						rxn->prdpos[i+1][j+1][1][0] = rpar;
					}
				}
				else {
					rxnr = sim->rxnss[orderr]->rxn[rr];
					if (rxnr->bindrad2[i + 1][j + 1]>=0)
						bindradr = sqrt(rxnr->bindrad2[i + 1][j + 1]);
					else
						bindradr = -1;

					if (dsum <= 0) {
						sprintf(erstr,
								"Cannot set unbinding distance because sum of product diffusion constants is 0");
						er = 4;
					}
					else if (rparamt == RPnone) {
						sprintf(erstr,
								"Undefined product parameter for reversible reaction");
						er = 5;
					}
					else if (rparamt == RPunbindrad) {
						rxn->unbindrad[i + 1][j + 1] = rpar;
						rxn->prdpos[i+1][j+1][0][0] = rpar * dc1 / dsum;
						rxn->prdpos[i+1][j+1][1][0] = -rpar * dc2 / dsum;
					}
					else if (rparamt == RPbounce) {
						rxn->unbindrad[i + 1][j + 1] = rpar;
						rxn->prdpos[i+1][j+1][0][0] = rpar * dc1 / dsum;
						rxn->prdpos[i+1][j+1][1][0] = -rpar * dc2 / dsum;
					}
					else if (rxnr->bindrad2[i + 1][j + 1] < 0) {
						sprintf(erstr,
								"Binding radius of reaction products is undefined");
						er = 6;
					}
					else if (rparamt == RPratio || rparamt == RPratio2) {
						rxn->unbindrad[i + 1][j + 1] = rpar * bindradr;
						rxn->prdpos[i+1][j+1][0][0] = rpar * bindradr * dc1 / dsum;
						rxn->prdpos[i+1][j+1][1][0] = -rpar * bindradr * dc2 / dsum;
					}
					else if (rparamt == RPpgem || rparamt == RPpgem2) {
						rpar= unbindingradius(rpar, sim->dt, dsum, bindradr);
						if (rpar == -2) {
							sprintf(erstr,
									"Cannot create an unbinding radius due to illegal input values");
							er = 7;
						}
						else if (rpar < 0) {
							sprintf(
									erstr,
									"Maximum possible geminate binding probability is %g",
									-rpar);
							er = 8;
						}
						else {
							rxn->unbindrad[i + 1][j + 1] = rpar;
							rxn->prdpos[i+1][j+1][0][0] = rpar * dc1 / dsum;
							rxn->prdpos[i+1][j+1][1][0] = -rpar * dc2 / dsum;
						}
					}
					else if (rparamt == RPpgemmax || rparamt == RPpgemmaxw
							|| rparamt == RPpgemmax2) { 
						rpar = unbindingradius(rpar, sim->dt, dsum, bindradr);  
						if (rpar == -2) {
							sprintf(erstr, "Illegal input values");
							er = 9;
						}
						else if (rpar <= 0) {
							rxn->unbindrad[i + 1][j + 1] = 0;
						}
						else if (rpar > 0) {
							rxn->unbindrad[i + 1][j + 1] = rpar;
							rxn->prdpos[i+1][j+1][0][0] = rpar * dc1 / dsum;
							rxn->prdpos[i+1][j+1][1][0] = -rpar * dc2 / dsum;
						}
						
					}
					printf("unbind: %f\n", rxn->unbindrad[i+1][j+1]);
					
				}
				
			}
		}
	}
	return er;
}

/* rxnsetproducts */
int rxnsetproducts(simptr sim, int order, char *erstr) {
	rxnssptr rxnss;
	int r, er;

	if (!sim || order < 0 || order > MAXORDER || !erstr) {
		sprintf(erstr, "illegal inputs to setproducts function");
		return 0;
	}
	rxnss = sim->rxnss[order];
	if (!rxnss)
		return -1;
	for (r = 0; r < rxnss->totrxn; r++) {
		er = rxnsetproduct(sim, order, r, erstr);
		if (er)
			return r;
	}
	return -1;
}

/* rxncalcrate.  Calculates the macroscopic rate constant using the microscopic
parameters that are stored in the reaction data structure.  All going well,
these results should exactly match those that were requested initially, although
this routine is useful as a check, and for situations where the microscopic
values were input rather than the mass action rate constants.  For bimolecular
reactions that are reversible, the routine calculates rates with accounting for
reversibility if the reversible parameter type of the reverse reaction is
RPpgem, RPpgemmax, RPratio, RPoffset, or RPnone, and not otherwise.  A value of
-1 is returned if input parameters are illegal and a value of 0 is returned if
the microscopic values for the indicated reaction are undefined (<0).  If the
input reaction has a reverse reaction or a continuation reaction, and ptr is
not input as NULL, then *pgemptr is set to the probability of geminate
recombination of the products; if there is no reverse or continuation reaction,
its value is set to -1. */
//Chr: k and l, surface indices, start from -1 for the nosurface option
double rxncalcrate(simptr sim, int order, int r, double *pgptr, int k, int l) { 
        printf("k: %i, l %i\n", k, l);
	rxnssptr rxnss;
	double ans, vol; 
	int i1, i2, i, j, r2, rev, o2, permit, found;
	double sum, sum2, flt2, step, a, bval;
	rxnptr rxn, rxnr;
	enum MolecState ms1, ms2, statelist[MAXORDER];
	enum RevParam rparamt;
	ans = -1;
	
	if (!sim)
		return -1;
	rxnss = sim->rxnss[order];
	if (!rxnss || r < 0 || r >= rxnss->totrxn)
		return -1;
	rxn = rxnss->rxn[r];

	
	if (order == 0) { // order 0
		if (rxn->cmpt)
			vol = rxn->cmpt->volume;
		else if (rxn->srf)
			vol = surfacearea(rxn->srf, sim->dim, NULL);
		else
			vol = systemvolume(sim);
		if (rxn->prob[k+1][l+1] < 0)
			ans = 0;
		else
			ans = rxn->prob[k+1][l+1] / sim->dt / vol;
	}

	else if (order == 1) { // order 1
		if (rxn->prob[k+1][l+1] < 0)
			ans = 0;
		else {
			i1 = rxn->rctident[0];
			sum = sum2 = 0;
			found = 0;
			for (j = 0; j < rxnss->nrxn[i1]; j++) {
				r2 = rxnss->table[i1][j];
				if (r2 == r)
					found = 1;
				else if (!found)
					sum2 += rxnss->rxn[r2]->prob[k+1][l+1] >= 0 ? rxnss->rxn[r2]->prob[k+1][l+1]
							* (1.0 - sum2) : 0;
				sum += rxnss->rxn[r2]->prob[k+1][l+1] >= 0 ? rxnss->rxn[r2]->prob[k+1][l+1] * (1.0
						- sum) : 0;
			}
			flt2 = -log(1.0 - sum) / sim->dt;
		      ans = rxn->prob[k+1][l+1] > 0 ? rxn->prob[k+1][l+1] * (1.0 - sum2) / sum * flt2 : 0;
		}
	}
	
	else if (order == 2) { // order 2
				if (rxn->bindrad2[k+1][l+1] < 0 || rxn->prob[k+1][l+1] < 0)
					ans = 0;
				else {
					i1 = rxn->rctident[0];
					i2 = rxn->rctident[1];
					i = rxnpackident(order, rxnss->maxspecies, rxn->rctident);
					for (j = 0; j < rxnss->nrxn[i] && rxnss->table[i][j] != r; j++)
						;
					if (rxnss->table[i][j] != r)
						return -1;
					permit = rxnreactantstate(rxn, statelist, 1);
					ms1 = statelist[0];
					ms2 = statelist[1];
					if (!permit)
						return 0;
					if (rxn->rparamt == RPconfspread)
						return -log(1.0 - rxn->prob[k+1][l+1]) / sim->dt;
					step = sqrt(2.0 * MolCalcDifcSum(sim, i1, ms1, i2, ms2, k
							, l) * sim->dt);
					a = sqrt(rxn->bindrad2[k+1][l+1]);
					rev = findreverserxn(sim, order, r, &o2, &r2);
					if (rev == 1)
						rparamt = sim->rxnss[o2]->rxn[r2]->rparamt;
					else
						rparamt = RPnone;
					if (rparamt == RPpgem || rparamt == RPbounce || rparamt
							== RPpgemmax || rparamt == RPpgemmaxw || rparamt
							== RPratio || rparamt == RPoffset || rparamt
							== RPfixed) {
						rxnr = sim->rxnss[o2]->rxn[r2];
						bval = distanceVVD(rxnr->prdpos[k+1][l+1][0], rxnr->prdpos[k+1][l+1][1],
								sim->dim);
						ans = numrxnrate(step, a, bval);
					}
					else
						ans = numrxnrate(step, a, -1);
					ans /= sim->dt;
					if (i1 == i2)
						ans /= 2.0;
					if (!rxn->permit[MSsoln * MSMAX1 + MSsoln])
						ans /= 2.0;
				
			
		}
	}

	else
		ans = 0;
	
	if (pgptr) 
	{ 
		if (rxn->nprod != 2 || findreverserxn(sim, order, r, &o2, &r2) == 0){
			*pgptr= -1;  
		}
		else
		{  
			step = sqrt(2.0 * MolCalcDifcSum(sim, rxn->prdident[0],
			rxn->prdstate[0], rxn->prdident[1], rxn->prdstate[1], k, l)
				* sim->dt); 
			bval = distanceVVD(rxn->prdpos[k+1][l+1][0], rxn->prdpos[k+1][l+1][1], sim->dim);
			rxnr = sim->rxnss[o2]->rxn[r2];
			a = sqrt(rxnr->bindrad2[k+1][l+1]);
			*pgptr = 1.0 - numrxnrate(step, a, -1)
						    / numrxnrate(step, a, bval); 
					
		}
			
		
	}

	return ans;
}

/* rxncalctau.  Calculates characteristic times for all reactions of order order
and stores them in the rxn->tau structure elements.  These are ignored for 0th
order reactions, are 1/k for first order reactions, are are [A][B]/[k([A]+[B])]
for second order reactions.  The actual calculated rate constant is used, not
the requested ones.  For second order, the current average concentrations are
used, which does not capture effects from spatial localization or concentration
changes.  For bimolecular reactions, if multiple reactant pairs map to the same
reaction, only the latter ones found are recorded.  Also, all molecule states
are counted, which ignores the permit reaction structure element. */
void rxncalctau(simptr sim, int order) {
	rxnssptr rxnss;
	rxnptr rxn;
	int r,i,j;
	double rate, vol, conc1, conc2;

	rxnss = sim->rxnss[order];
	if (!rxnss)
		return;

	if (order == 1) {
		for (r = 0; r < rxnss->totrxn; r++) {
			rxn = rxnss->rxn[r];
			for(i=0;i<=sim->srfss->nsrf;i++){
			    for(j=0;j<=sim->srfss->nsrf;j++){
				rate = rxncalcrate(sim, 1, r, NULL, i-1,j-1);
				rxn->tau[i][j] = 1.0 / rate;
			    }
			}
		}
	}

	else if (order == 2) {
		vol = systemvolume(sim);
		for (r = 0; r < rxnss->totrxn; r++) {
			rxn = rxnss->rxn[r];
			conc1 = (double) molcount(sim, rxn->rctident[0], MSall, NULL, -1)
					/ vol;
			conc2 = (double) molcount(sim, rxn->rctident[1], MSall, NULL, -1)
					/ vol;
			
			for(i=0;i<=sim->srfss->nsrf;i++){
			    for(j=0;j<=sim->srfss->nsrf;j++){
				rate = rxncalcrate(sim, 2, r, NULL,i-1,j-1);
				if (rxn->rparamt == RPconfspread)
				    rxn->tau[i][j] = 1.0 / rate;
			else
				rxn->tau[i][j] = (conc1 + conc2) / (rate * conc1 * conc2);
			    }
			}
		}
	}

	return;
}

/* rxnsettimestep.  Sets reaction structure parameters for the simulation time
step.  Return values are 0 for success, 1 for an error with setting either rates
or products (and output to stderr with an error message), or 2 if the reaction
structure was not sufficiently set up beforehand. */
int rxnsettimestep(simptr sim) {
	int er, order, wflag;
	char errorstr[STRCHAR];

	er = 0;
	for (order = 0; order < MAXORDER && er <= 0; order++) {
		if (sim->rxnss[order] && sim->rxnss[order]->condition < SCparams)
			er = 1;
		else if (!sim->rxnss[order] || sim->rxnss[order]->condition == SCok)
			er--;
	}
	if (er == -(MAXORDER + 1))
		return 0;
	if (er == 1)
		return 2;
	er = 0;

	wflag = strchr(sim->flags, 'w') ? 1 : 0;
	for (order = 0; order < MAXORDER; order++)
		if (sim->rxnss[order] && sim->rxnss[order]->condition <= SCparams) {
			er = rxnsetrates(sim, order, errorstr); // set rates
			if (er >= 0) {
				fprintf(
						stderr,
						"Error setting rate for reaction order %i, reaction %s\n",
						order, sim->rxnss[order]->rname[er]);
				fprintf(stderr, "%s\n", errorstr);
				return 1;
			}
		}

	for (order = 0; order < MAXORDER; order++)
		if (sim->rxnss[order] && sim->rxnss[order]->condition <= SCparams) {
			errorstr[0] = '\0';
			er = rxnsetproducts(sim, order, errorstr); // set products
			if (er >= 0) {
				fprintf(
						stderr,
						"Error setting products for reaction order %i, reaction %s\n",
						order, sim->rxnss[order]->rname[er]);
				fprintf(stderr, "%s\n", errorstr);
				return 1;
			}
			if (!wflag && strlen(errorstr))
				fprintf(stderr, "%s\n", errorstr);
		}

	for (order = 0; order < MAXORDER; order++) // calculate tau values
		if (sim->rxnss[order] && sim->rxnss[order]->condition <= SCparams)
			rxncalctau(sim, order);

	rxnsetcondition(sim, -1, SCok, 1);
	return 0;
}

/******************************************************************************/
/****************************** structure set up ******************************/
/******************************************************************************/

/* rxnsetcondition */
void rxnsetcondition(simptr sim, int order, enum StructCond cond, int upgrade) {
	int o1, o2;

	if (!sim)
		return;
	if (order < 0) {
		o1 = 0;
		o2 = 2;
	}
	else if (order <= 2)
		o1 = o2 = order;
	else
		return;

	for (order = o1; order <= o2; order++) {
		if (sim->rxnss[order]) {
			if (upgrade == 0 && sim->rxnss[order]->condition > cond)
				sim->rxnss[order]->condition = cond;
			else if (upgrade == 1 && sim->rxnss[order]->condition < cond)
				sim->rxnss[order]->condition = cond;
			else if (upgrade == 2)
				sim->rxnss[order]->condition = cond;
			if (sim->rxnss[order]->condition < sim->condition) {
				cond = sim->rxnss[order]->condition;
				simsetcondition(sim, cond == SCinit ? SClists : cond, 0);
			}
		}
	}

	return;
}

/* rxnsetmollist */
int rxnsetmollist(simptr sim, int order) {
	rxnssptr rxnss;
	int maxlist, ll, nl2o, r, i1, i2, ll1, ll2;
	rxnptr rxn;
	enum MolecState ms1, ms2;

	rxnss = sim->rxnss[order];
	if (!rxnss)
		return 0;
	if (rxnss->condition > SClists)
		return 0;
	if (order == 0) {
		rxnsetcondition(sim, order, SCparams, 1);
		return 0;
	}
	if (order > 2)
		return 2;
	if (!sim->mols || sim->mols->condition <= SClists)
		return 3;

	maxlist = rxnss->maxlist;
	if (maxlist != sim->mols->maxlist) {
		free(rxnss->rxnmollist);
		rxnss->rxnmollist = NULL;
		maxlist = sim->mols->maxlist;
		if (maxlist > 0) {
			nl2o = intpower(maxlist, order);
			rxnss->rxnmollist = (int*) calloc(nl2o, sizeof(int));
			if (!rxnss->rxnmollist)
				return 1;
		}
		rxnss->maxlist = maxlist;
	}

	if (maxlist > 0) {
		nl2o = intpower(maxlist, order);
		for (ll = 0; ll < nl2o; ll++)
			rxnss->rxnmollist[ll] = 0;

		for (r = 0; r < rxnss->totrxn; r++) {
			rxn = rxnss->rxn[r];
			i1 = rxn->rctident[0];
			if (order == 1) {
				for (ms1 = 0; ms1 < MSMAX1; ms1++)
					if (rxn->permit[ms1] && (rxn->prob[0][0] > 0 || rxn->rate > 0)) {
						ll1 = sim->mols->listlookup[i1][ms1];
						rxnss->rxnmollist[ll1] = 1;
					}
			}
			else if (order == 2) {
				i2 = rxn->rctident[1];
				for (ms1 = 0; ms1 < MSMAX1; ms1++)
					for (ms2 = 0; ms2 < MSMAX1; ms2++)
						//default binding radius should be above zero in any case
						if (rxn->permit[ms1 * MSMAX1 + ms2] && rxn->prob[0][0] != 0
								&& (rxn->rate > 0 || rxn->bindrad2[0][0] > 0)) {
							ll1
									= sim->mols->listlookup[i1][ms1 == MSbsoln ? MSsoln
											: ms1];
							ll2
									= sim->mols->listlookup[i2][ms2 == MSbsoln ? MSsoln
											: ms2];
							rxnss->rxnmollist[ll1 * maxlist + ll2] = 1;
							rxnss->rxnmollist[ll2 * maxlist + ll1] = 1;
						}
			}
		}
	}

	rxnsetcondition(sim, order, SCparams, 1);
	return 0;
}

/* RxnSetValue.  Sets certain options of the reaction structure for reaction
rxn to value.  Returns 0 for success, 1 for missing input item, 2 for unknown
option, 3 for a value that was set previously, or 4 for an illegal value (e.g.
a negative rate).  In most cases, the value is set as requested, despite the
error message.  If option is "rate", the rate element is set; if option is
"confspreadrad", the reaction is made confspread and the squared binding radius is
set. */
int RxnSetValue(simptr sim, char *option, rxnptr rxn, double value) {
	int er, k , l; 
	er = 0;
	for (k = 0; k <= sim->nrfs; k++) {
		for(l = 0; l<=sim->nrfs; l++) {
		    if (!rxn || !option)
			er = 1;
		    else if (!strcmp(option, "rate")) {
		    if (rxn->rate != -1)
			er = 3;
		    if (value < 0)
			er = 4;
			rxn->rate = value;
		    }
		    else if (!strcmp(option, "confspreadrad")) {
			if (rxn->rparamt == RPconfspread)
			  er = 3;
			  rxn->rparamt = RPconfspread;
			if (value < 0)
			    er = 4;
			    rxn->bindrad2[k][l] = value * value;
		    }
	
		    else if (!strcmp(option, "bindrad")) {
			if (rxn->rparamt == RPconfspread)
			  er = 3;
			if (value < 0)
			  er = 4;
			  rxn->bindrad2[k][l] = value * value;
			
		    }
		    else if (!strcmp(option, "prob")) {
		      if (value < 0)
			er = 4;
			if (rxn->rxnss->order > 0 && value > 1)
			  er = 4;
			rxn->prob[k][l] = value;
		    }
		    else
			er = 2; 
		}
	}
	rxnsetcondition(sim, -1, SCparams, 0);
	return er;
}

/* RxnSetRevparam.  Sets the reversible parameter type and the appropriate
reversible parameters for reaction rxn.  The parameter type, rxn->paramt, is set
to rparamt.  If rparamt requires a single value, which is stored in rxn->rparam, it
is sent in with rparam.  Otherwise, for fixed and offset parameter types, send
in the product number that is being altered with prd, the vector with pos, and
the system dimensionality with dim.  This returns 0 for success, 1 as a warning
that the reversible parameter type has been set before (except for offset and
fixed types, where many different products need to be set), or 2 for parameters
that are out of bounds. */
int RxnSetRevparam(simptr sim, rxnptr rxn, enum RevParam rparamt,
		double rparam, int prd, double *pos, int dim) {
	int d, er, sf1, sf2;

	er = 0;
	if (rxn->rparamt != RPnone)
		er = 1;
	rxn->rparamt = rparamt;

	if (rparamt == RPpgem || rparamt == RPpgemmax || rparamt == RPpgem2
			|| rparamt == RPpgemmax2) {
		if (!(rparam > 0 && rparam <= 1))
			er = 2;
		rxn->rparam = rparam;
	}
	else if (rparamt == RPbounce || rparamt == RPratio || rparamt
			== RPunbindrad || rparamt == RPratio2) {
		if (rparam < 0)
			er = 2;
		rxn->rparam = rparam;
	}
	else if (rparamt == RPoffset || rparamt == RPfixed) {
		for (d = 0; d < dim; d++)
			for(sf1=0;sf1<sim->nrfs;sf1++){
			  for(sf2=0;sf2<sim->nrfs;sf2++){
			    rxn->prdpos[sf1][sf2][prd][d] = pos[d];
			  }
			}
	}
	rxnsetcondition(sim, -1, SCparams, 0);
	return er;
}

/* RxnSetPermit.  Sets the permit element of reaction rxn, which has order
order, for the states that are included in rctstate to value value.  value
should be 0 to set permissions to forbidden or 1 to set permissions to
permitted.  Each item of rctstate may be an individual state or may be MSall.
Other values are not allowed (and are not caught here).  This does not affect
other permit elements. */
void RxnSetPermit(simptr sim, rxnptr rxn, int order, enum MolecState *rctstate,
		int value) {
	enum MolecState ms, nms2o, mslist[MSMAX1];
	int set, ord;
	static int recurse;

	if (order == 0)
		return;
	nms2o = intpower(MSMAX1, order);
	for (ms = 0; ms < nms2o; ms++) {
		rxnunpackstate(order, ms, mslist);
		set = 1;
		for (ord = 0; ord < order && set; ord++)
			if (!(rctstate[ord] == MSall || rctstate[ord] == mslist[ord]))
				set = 0;
		if (set)
			rxn->permit[ms] = value;
	}

	if (order == 2 && rxn->rctident[0] == rxn->rctident[1] && recurse == 0) {
		recurse = 1;
		mslist[0] = rctstate[1];
		mslist[1] = rctstate[0];
		RxnSetPermit(sim, rxn, order, mslist, value);
		recurse = 0;
	}

// 	rxnsetcondition(sim, -1, SCparams, 0);
	surfsetcondition(sim->srfss, SClists, 0);
	return;
}

/* RxnAddReaction.  Adds a reaction to the simulation, including all necessary
memory allocation.  rname is the name of the reaction, order is the order of the
reaction, and nprod is the number of products.  rctident and rctstate are
vectors of size order that contain the reactant identities and states,
respectively.  Likewise, prdident and prdstate are vectors of size nprod that
contain the product identities and states.  This returns the just added reaction
for success and NULL for inability to allocate memory.  This allocates reaction
superstuctures and reaction structures, and will enlarge any array, as needed. */
rxnptr RxnAddReaction(simptr sim, char *rname, int order, int *rctident,
		enum MolecState *rctstate, int nprod, int *prdident,
		enum MolecState *prdstate, compartptr cmpt, surfaceptr srf) {
	char **newrname;
	rxnptr *newrxn;
	int *newtable, identlist[MAXORDER];
	rxnssptr rxnss;
	rxnptr rxn;
	int maxrxn, maxspecies, i, j, r, rct, prd, d, k, done, freerxn, ns, ns2;

	rxnss = NULL;
	rxn = NULL;
	newrname = NULL;
	newrxn = NULL;
	newtable = NULL;
	maxrxn = 0;
	freerxn = 1;

	if (!sim->rxnss[order]) {
		CHECK(sim->mols)
;		CHECK(sim->rxnss[order]=rxnssalloc(order,sim->mols->maxspecies));
		sim->rxnss[order]->sim=sim;
		rxnsetcondition(sim,order,SCinit,0);
		rxnsetcondition(sim,-1,SClists,0);}
	rxnss=sim->rxnss[order];
	maxspecies=rxnss->maxspecies;
	r=stringfind(rxnss->rname,rxnss->totrxn,rname);

	if(r>=0) {
		CHECK(rxnss->rxn[r]->nprod==0);
		rxn=rxnss->rxn[r];}
	else {
		if(rxnss->totrxn==rxnss->maxrxn) { // make more reaction space in superstructure, if needed
			maxrxn=(rxnss->maxrxn>0)?2*rxnss->maxrxn:2;
			CHECK(newrname=(char**)calloc(maxrxn,sizeof(char*)));
			for(r=0;r<rxnss->maxrxn;r++) newrname[r]=rxnss->rname[r];
			for(r=rxnss->maxrxn;r<maxrxn;r++) newrname[r]=NULL;
			for(r=rxnss->maxrxn;r<maxrxn;r++) CHECK(newrname[r]=EmptyString());
			CHECK(newrxn=(rxnptr*)calloc(maxrxn,sizeof(rxnptr)));
			for(r=0;r<rxnss->maxrxn;r++) newrxn[r]=rxnss->rxn[r];
			for(r=rxnss->maxrxn;r<maxrxn;r++) newrxn[r]=NULL;
			rxnss->maxrxn=maxrxn;
			free(rxnss->rname);
			rxnss->rname=newrname;
			newrname=NULL;
			free(rxnss->rxn);
			rxnss->rxn=newrxn;
			newrxn=NULL;}

		CHECK(rxn=rxnalloc(order, sim->nrfs)); // create reaction and set up reactants
		rxn->rxnss=rxnss;
		rxn->rname=rxnss->rname[rxnss->totrxn];
		if(order>0) {
			for(rct=0;rct<order;rct++) rxn->rctident[rct]=rctident[rct];
			for(rct=0;rct<order;rct++) rxn->rctstate[rct]=rctstate[rct];
			RxnSetPermit(sim,rxn,order,rctstate,1);}

		if(order>0) { // set up nrxn and table
			k=0;
			done=0;
			while(!done) {
				k=Zn_permute(rctident,identlist,order,k);
				if(k==-1) {fprintf(stderr,"SMOLDYN BUG: Zn_permute.\n");exit(0);}
				if(k==0) done=1;
				i=rxnpackident(order,maxspecies,identlist);
				CHECK(newtable=(int*)calloc(rxnss->nrxn[i]+1,sizeof(int)));
				for(j=0;j<rxnss->nrxn[i];j++) newtable[j]=rxnss->table[i][j];
				newtable[j]=rxnss->totrxn;
				free(rxnss->table[i]);
				rxnss->table[i]=newtable;
				newtable=NULL;
				rxnss->nrxn[i]++;}}

		strncpy(rxnss->rname[rxnss->totrxn],rname,STRCHAR-1); // plug in reaction
		rxnss->rname[rxnss->totrxn][STRCHAR-1]='\0';
		rxnss->totrxn++;
		rxnss->rxn[rxnss->totrxn-1]=rxn;}
	freerxn=0;

	rxn->nprod=nprod; // set up products
	if(nprod) {
		CHECK(rxn->prdident=(int*)calloc(nprod,sizeof(int)));
		    for(prd=0;prd<nprod;prd++) 
		      rxn->prdident[prd]=prdident[prd];
		CHECK(rxn->prdstate=(enum MolecState*)calloc(nprod,sizeof(enum MolecState)));
		    for(prd=0;prd<nprod;prd++) 
		      rxn->prdstate[prd]=prdstate[prd];
		    
		//Chr: Sorry for this ugly 4dim array...
		CHECK(rxn->prdpos=(double****)calloc(sim->nrfs, sizeof(double***)));
		for(ns=0;ns<sim->nrfs+1; ns++){
		      rxn->prdpos[ns]=NULL; 
		      CHECK(rxn->prdpos[ns]=(double***) calloc(sim->nrfs+1, sizeof(double **)));
		      for(ns2=0;ns2<sim->nrfs+1;ns2++) {
			  rxn->prdpos[ns][ns2]=NULL;
			  CHECK(rxn->prdpos[ns][ns2]=(double**)calloc(nprod,sizeof(double*)));
			  for(prd=0;prd<nprod;prd++) {
			      rxn->prdpos[ns][ns2][prd]=NULL;
			      CHECK(rxn->prdpos[ns][ns2][prd]=(double*)calloc(sim->dim,sizeof(double)));
			      for(d=0;d<sim->dim;d++) rxn->prdpos[ns][ns2][prd][d]=0;
			  }
		      }
		}
	}
	rxn->cmpt=cmpt; // add reaction compartment
	rxn->srf=srf; // add reaction surface
	surfsetcondition(sim->srfss,SClists,0);
	return rxn;

	failure:
	if(!rxnss) return NULL;
	if(newrname) {
		for(r=rxnss->maxrxn;r<maxrxn;r++) free(newrname[r]);
		free(newrname);}
	free(newrxn);
	if(freerxn) rxnfree(rxn);
	return NULL;}

/* RxnAddReactionCheck */
rxnptr RxnAddReactionCheck(simptr sim, char *rname, int order, int *rctident,
		enum MolecState *rctstate, int nprod, int *prdident,
		enum MolecState *prdstate, compartptr cmpt, surfaceptr srf, char *erstr) {
	rxnptr rxn;
	int i;

	CHECKS(sim,"sim undefined")
;	CHECKS(sim->mols,"sim is missing molecule superstructure");
	CHECKS(rname,"rname is missing");
	CHECKS(strlen(rname)<STRCHAR,"rname is too long");
	CHECKS(order>=0 && order<=2,"order is out of bounds");
	if(order>0) {
		CHECKS(rctident,"rctident is missing");}
	for(i=0;i<order;i++) {
		CHECKS(rctident[i]>0 && rctident[i]<sim->mols->nspecies,"reactant identity out of bounds");
		CHECKS(rctstate[i]>=0 && rctstate[i]<MSMAX1,"reactant state out of bounds");}
	CHECKS(nprod>=0,"nprod out of bounds");
	for(i=0;i<nprod;i++) {
		CHECKS(prdident[i]>0 && prdident[i]<sim->mols->nspecies,"reactant identity out of bounds");
		CHECKS(prdstate[i]>=0 && prdstate[i]<MSMAX1,"reactant state out of bounds");}
	if(cmpt) {
		CHECKS(sim->cmptss,"sim is missing compartment superstructure");}
	if(srf) {
		CHECKS(sim->srfss,"sim is missing surface superstructure");}
	rxn=RxnAddReaction(sim,rname,order,rctident,rctstate,nprod,prdident,prdstate,cmpt,srf);
	return rxn;
	failure:
	return NULL;}

/* loadrxn.  Loads a reaction structure from an already opened disk file
described with pfpptr.  If successful, it returns 0 and the reaction is added to
sim.  Otherwise it returns 1 and error information in pfpptr.  If a reaction
structure of the same order has already been set up, this function can use it
and add more reactions to it.  It can also allocate and set up a new structure,
if needed.  This need for this function has been largely superceded by
functionality in loadsim, but this is kept for backward compatibility. */
int loadrxn(simptr sim, ParseFilePtr *pfpptr, char *line2, char *erstr) {
	ParseFilePtr pfp;
	rxnssptr rxnss;
	int order, got[2], itct, maxspecies, done, pfpcode, firstline2;
	char word[STRCHAR], errstring[STRCHAR];

	char nm[STRCHAR], nm2[STRCHAR], rxnnm[STRCHAR];
	int i, r, prd, j,k,l, i1, i2, i3, nptemp, identlist[MAXPRODUCT], d;
	double rtemp, postemp[DIMMAX];
	enum MolecState ms, ms1, ms2, mslist[MAXPRODUCT];
	rxnptr rxn;
	enum RevParam rparamt;

	pfp = *pfpptr;
	setstdZV(got, 2, 0);
	order = -1;
	rxnss = NULL;
	maxspecies = sim->mols->maxspecies;
	done = 0;
	firstline2 = line2 ? 1 : 0;

	while (!done) {
		if (pfp->lctr == 0 && !strchr(sim->flags, 'q'))
			printf(" Reading file: '%s'\n", pfp->fname);
		if (firstline2) {
			strcpy(word, "order");
			pfpcode = 1;
			firstline2 = 0;
		}
		else
			pfpcode = Parse_ReadLine(&pfp, word, &line2, errstring);
		*pfpptr = pfp;
		CHECKS(pfpcode!=3,errstring)
;		if(pfpcode==0); // already taken care of

		else if(pfpcode==2) { // end reading
			done=1;}

		else if(pfpcode==3) { // error
			CHECKS(0,"SMOLDYN BUG: parsing error");}

		else if(!strcmp(word,"end_reaction")) { // end_reaction
			CHECKS(!line2,"unexpected text following end_reaction");
			return 0;}

		else if(!line2) { // just word
			CHECKS(0,"unknown word or missing parameter");}

		else if(!strcmp(word,"order")) { // order, got[0]
			CHECKS(!got[0],"order can only be entered once");
			got[0]++;
			itct=sscanf(line2,"%i",&order);
			CHECKS(itct==1,"error reading order");
			CHECKS(order>=0 && order<=2,"order needs to be between 0 and 2");
			if(!sim->rxnss[order]) {
				CHECKS(sim->rxnss[order]=rxnssalloc(order,maxspecies),"out of memory creating reaction superstructure");
				rxnsetcondition(sim,order,SCinit,0);
				sim->rxnss[order]->sim=sim;}
			rxnss=sim->rxnss[order];
			CHECKS(!strnword(line2,2),"unexpected text following order");}

		else if(!strcmp(word,"max_rxn")) { // max_rxn, got[1]
		}

		else if(!strcmp(word,"reactant") && order==0) { // reactant, 0
			CHECKS(got[0],"order needs to be entered before reactant");
			j=wordcount(line2);
			CHECKS(j>0,"number of reactions needs to be >0");
			for(j--;j>=0;j--) {
				itct=sscanf(line2,"%s",nm);
				CHECKS(itct==1,"missing reaction name in reactant");
				CHECKS(stringfind(rxnss->rname,rxnss->totrxn,nm)<0,"reaction name has already been used");
				CHECKS(RxnAddReaction(sim,nm,0,NULL,NULL,0,NULL,NULL,NULL,NULL),"faied to add 0th order reaction");
				line2=strnword(line2,2);}}

		else if(!strcmp(word,"reactant") && order==1) { // reactant, 1
			CHECKS(got[0],"order needs to be entered before reactant");
			i=readmolname(sim,line2,&ms);
			CHECKS(i!=0,"empty molecules cannot react");
			CHECKS(i!=-1,"reactant format: name[(state)] rxn_name");
			CHECKS(i!=-2,"mismatched or improper parentheses around molecule state");
			CHECKS(i!=-3,"cannot read molecule state value");
			CHECKS(i!=-4,"molecule name not recognized");
			CHECKS(i!=-5,"molecule name cannot be set to 'all'");
			CHECKS(ms!=MSbsoln,"bsoln is not an allowed state for first order reactants");
			identlist[0]=i;
			mslist[0]=ms;
			CHECKS(line2=strnword(line2,2),"no reactions listed");
			j=wordcount(line2);
			for(j--;j>=0;j--) {
				itct=sscanf(line2,"%s",nm);
				CHECKS(itct==1,"missing reaction name in reactant");
				CHECKS(stringfind(rxnss->rname,rxnss->totrxn,nm)<0,"reaction name has already been used");
				CHECKS(RxnAddReaction(sim,nm,1,identlist,mslist,0,NULL,NULL,NULL,NULL),"faied to add 1st order reaction");
				line2=strnword(line2,2);}}

		else if(!strcmp(word,"reactant") && order==2) { // reactant, 2
			CHECKS(got[0],"order needs to be entered before reactants");
			i1=readmolname(sim,line2,&ms1);
			CHECKS(i1!=0,"empty molecules cannot react");
			CHECKS(i1!=-1,"reactant format: name[(state)] + name[(state)] rxn_name");
			CHECKS(i1!=-2,"mismatched or improper parentheses around molecule state");
			CHECKS(i1!=-3,"cannot read molecule state value");
			CHECKS(i1!=-4,"molecule name not recognized");
			CHECKS(i1!=-5,"molecule name cannot be set to 'all'");
			identlist[0]=i1;
			mslist[0]=ms1;
			CHECKS(line2=strnword(line2,3),"reactant format: name[(state)] + name[(state)] rxn_list");
			i2=readmolname(sim,line2,&ms2);
			CHECKS(i2!=0,"empty molecules cannot react");
			CHECKS(i2!=-1,"reactant format: name[(state)] + name[(state)] rxn_name value");
			CHECKS(i2!=-2,"mismatched or improper parentheses around molecule state");
			CHECKS(i2!=-3,"cannot read molecule state value");
			CHECKS(i2!=-4,"molecule name not recognized");
			CHECKS(i2!=-5,"molecule name cannot be set to 'all'");
			identlist[1]=i2;
			mslist[1]=ms2;
			CHECKS(line2=strnword(line2,2),"no reactions listed");
			j=wordcount(line2);
			for(j--;j>=0;j--) {
				itct=sscanf(line2,"%s",nm);
				CHECKS(itct==1,"missing reaction name in reactant");
				CHECKS(stringfind(rxnss->rname,rxnss->totrxn,nm)<0,"reaction name has already been used");
				CHECKS(RxnAddReaction(sim,nm,2,identlist,mslist,0,NULL,NULL,NULL,NULL),"faied to add 1st order reaction");
				line2=strnword(line2,2);}}

		else if(!strcmp(word,"permit") && order==0) { // permit, 0
			CHECKS(0,"reaction permissions are not allowed for order 0 reactions");}

		else if(!strcmp(word,"permit") && order==1) { // permit, 1
			CHECKS(got[0],"order needs to be entered before permit");
			i=readmolname(sim,line2,&ms);
			CHECKS(i!=0,"empty molecules cannot be entered");
			CHECKS(i!=-1,"permit format: name(state) rxn_name value");
			CHECKS(i!=-2,"mismatched or improper parentheses around molecule state");
			CHECKS(i!=-3,"cannot read molecule state value");
			CHECKS(i!=-4,"molecule name not recognized");
			CHECKS(i!=-5,"molecule name cannot be set to 'all'");
			CHECKS(ms<MSMAX,"all and bsoln are not allowed in permit for first order reactions");
			CHECKS(line2=strnword(line2,2),"permit format: name(state) rxn_name value");
			itct=sscanf(line2,"%s %i",rxnnm,&i3);
			CHECKS(itct==2,"permit format: name(state) rxn_name value");
			r=stringfind(rxnss->rname,rxnss->totrxn,rxnnm);
			CHECKS(r>=0,"in permit, reaction name not recognized");
			for(j=0;j<rxnss->nrxn[i] && rxnss->table[i][j]!=r;j++);
			CHECKS(rxnss->table[i][j]==r,"in permit, reaction was not already listed for this reactant");
			CHECKS(i3==0 || i3==1,"in permit, value needs to be 0 or 1");
			rxnss->rxn[r]->permit[ms]=i3;
			CHECKS(!strnword(line2,3),"unexpected text following permit");}

		else if(!strcmp(word,"permit") && order==2) { // permit, 2
			CHECKS(got[0],"order needs to be entered before permit");
			i1=readmolname(sim,line2,&ms);
			CHECKS(i1!=0,"empty molecules not allowed");
			CHECKS(i1!=-1,"permit format: name(state) + name(state) rxn_name value");
			CHECKS(i1!=-2,"mismatched or improper parentheses around first molecule state");
			CHECKS(i1!=-3,"cannot read first molecule state value");
			CHECKS(i1!=-4,"first molecule name not recognized");
			CHECKS(i1!=-5,"first molecule state missing, or is set to 'all'");
			CHECKS(ms<MSMAX1,"all is not allowed in permit");
			CHECKS(line2=strnword(line2,3),"permit format: name(state) + name(state) rxn_name value");
			i2=readmolname(sim,line2,&ms2);
			CHECKS(i2!=0,"empty molecules are not allowed");
			CHECKS(i2!=-1,"permit format: name(state) + name(state) rxn_name value");
			CHECKS(i2!=-2,"mismatched or improper parentheses around second molecule state");
			CHECKS(i2!=-3,"cannot read second molecule state value");
			CHECKS(i2!=-4,"second molecule name not recognized");
			CHECKS(i2!=-5,"second molecule state missing, or is set to 'all'");
			CHECKS(ms2<MSMAX1,"all is not allowed in permit");
			CHECKS(line2=strnword(line2,2),"permit format: name(state) + name(state) rxn_name value");
			i=i1*maxspecies+i2;
			itct=sscanf(line2,"%s %i",rxnnm,&i3);
			CHECKS(itct==2,"permit format: name(state) + name(state) rxn_name value");
			r=stringfind(rxnss->rname,rxnss->totrxn,rxnnm);
			CHECKS(r>=0,"in permit, reaction name not recognized");
			for(j=0;j<rxnss->nrxn[i] && rxnss->table[i][j]!=r;j++);
			CHECKS(rxnss->table[i][j]==r,"in permit, reaction was not already listed for this reactant");
			CHECKS(i3==0 || i3==1,"in permit, value needs to be 0 or 1");
			rxnss->rxn[r]->permit[ms*MSMAX1+ms2]=i3;
			CHECKS(!strnword(line2,3),"unexpected text following permit");}

		else if(!strcmp(word,"rate")) { // rate
			CHECKS(got[0],"order needs to be entered before rate");
			itct=sscanf(line2,"%s %lg",nm,&rtemp);
			CHECKS(itct==2,"format for rate: rxn_name rate");
			r=stringfind(rxnss->rname,rxnss->totrxn,nm);
			CHECKS(r>=0,"unknown reaction name in rate");
			CHECKS(rtemp>=0,"reaction rate needs to be >=0 (maybe try rate_internal)");
			rxnss->rxn[r]->rate=rtemp;
			CHECKS(!strnword(line2,3),"unexpected text following rate");}

		else if(!strcmp(word,"confspread_radius")) { // confspread_radius
			CHECKS(got[0],"order needs to be entered before confspread_radius");
			itct=sscanf(line2,"%s %lg",nm,&rtemp);
			CHECKS(itct==2,"format for confspread_radius: rxn_name radius");
			r=stringfind(rxnss->rname,rxnss->totrxn,nm);
			CHECKS(r>=0,"unknown reaction name in confspread_radius");
			CHECKS(rxnss->rxn[r]->rparamt!=RPconfspread,"confspread_radius can only be entered once for a reaction");
			CHECKS(rtemp>=0,"confspread_radius needs to be >=0");
			for(k=0;k<=sim->nrfs;k++){
			  for(l=0;l<=sim->nrfs;l++){
			    rxnss->rxn[r]->bindrad2[k][l]=rtemp*rtemp;
			  }
			}
			rxnss->rxn[r]->rparamt=RPconfspread;
			CHECKS(!strnword(line2,3),"unexpected text following confspread_radius");}

		else if(!strcmp(word,"rate_internal")) { // rate_internal
			CHECKS(got[0],"order needs to be entered before rate_internal");
			itct=sscanf(line2,"%s %lg",nm,&rtemp);
			CHECKS(itct==2,"format for rate_internal: rxn_name rate");
			r=stringfind(rxnss->rname,rxnss->totrxn,nm);
			CHECKS(r>=0,"unknown reaction name in rate_internal");
			CHECKS(rtemp>=0,"rate_internal needs to be >=0");
			for(k=0;k<=sim->nrfs;k++){
			    for(l=0;l<=sim->nrfs;l++){
			      if(order<2) rxnss->rxn[r]->prob[k][l]=rtemp; 

			      else {
			  
			      rxnss->rxn[r]->bindrad2[k][l]=rtemp*rtemp;
			    } 
			  } 
			}
			CHECKS(!strnword(line2,3),"unexpected text following rate_internal");}

		else if(!strcmp(word,"probability")) { // probability
			CHECKS(got[0],"order needs to be entered before probability");
			itct=sscanf(line2,"%s %lg",nm,&rtemp);
			CHECKS(itct==2,"format for probability: rxn_name probability");
			r=stringfind(rxnss->rname,rxnss->totrxn,nm);
			CHECKS(r>=0,"unknown reaction name in probability");
			CHECKS(rtemp>=0,"probability needs to be >=0");
			CHECKS(rtemp<=1,"probability needs to be <=1");
			for(k=0;k<=sim->nrfs;k++){
			    for(l=0;l<=sim->nrfs;l++){
				rxnss->rxn[r]->prob[k][l]=rtemp;
			    }
			}
			CHECKS(!strnword(line2,3),"unexpected text following probability");}

		else if(!strcmp(word,"product")) { // product
			CHECKS(got[0],"order needs to be entered before product");
			itct=sscanf(line2,"%s",rxnnm);
			CHECKS(itct==1,"format for product: rxn_name product_list");
			r=stringfind(rxnss->rname,rxnss->totrxn,rxnnm);
			CHECKS(r>=0,"unknown reaction name in product");
			nptemp=symbolcount(line2,'+')+1;
			CHECKS(nptemp>=0,"number of products needs to be >=0");
			CHECKS(nptemp<=MAXPRODUCT,"more products are entered than Smoldyn can handle");
			CHECKS(rxnss->rxn[r]->nprod==0,"products for a reaction can only be entered once");
			for(prd=0;prd<nptemp;prd++) {
				CHECKS(line2=strnword(line2,2),"product list is incomplete");
				i=readmolname(sim,line2,&ms);
				CHECKS(i!=0,"empty molecules cannot be products");
				CHECKS(i!=-1,"product format: rxn_name name(state) + name(state) + ...");
				CHECKS(i!=-2,"mismatched or improper parentheses around molecule state");
				CHECKS(i!=-3,"cannot read molecule state value");
				CHECKS(i!=-4,"molecule name not recognized");
				CHECKS(ms<MSMAX1,"product state is not allowed");
				identlist[prd]=i;
				mslist[prd]=ms;
				if(prd+1<nptemp) {
					CHECKS(line2=strnword(line2,2),"incomplete product list");}}
			CHECKS(RxnAddReaction(sim,rxnnm,order,NULL,NULL,nptemp,identlist,mslist,NULL,NULL),"failed to add products to reaction");
			CHECKS(!strnword(line2,2),"unexpected text following product");}

		else if(!strcmp(word,"product_param")) { // product_param
			CHECKS(got[0],"order needs to be entered before product_param");
			itct=sscanf(line2,"%s",nm);
			CHECKS(itct==1,"format for product_param: rxn type [parameters]");
			r=stringfind(rxnss->rname,rxnss->totrxn,nm);
			CHECKS(r>=0,"unknown reaction name in product_param");
			rxn=rxnss->rxn[r];
			rparamt=rxn->rparamt;
			CHECKS(rparamt==RPnone,"product_param can only be entered once");
			CHECKS(line2=strnword(line2,2),"format for product_param: rxn type [parameters]");
			itct=sscanf(line2,"%s",nm);
			CHECKS(itct==1,"missing parameter type in product_param");
			rparamt=rxnstring2rp(nm);
			CHECKS(rparamt!=RPnone,"unrecognized parameter type");
			rtemp=0;
			prd=0;
			for(d=0;d<sim->dim;d++) postemp[prd]=0;
			if(rparamt==RPpgem || rparamt==RPpgemmax || rparamt==RPratio || rparamt==RPunbindrad || rparamt==RPpgem2 || rparamt==RPpgemmax2 || rparamt==RPratio2) {
				CHECKS(line2=strnword(line2,2),"missing parameter in product_param");
				itct=sscanf(line2,"%lg",&rtemp);
				CHECKS(itct==1,"error reading parameter in product_param");}
			else if(rparamt==RPoffset || rparamt==RPfixed) {
				CHECKS(line2=strnword(line2,2),"missing parameters in product_param");
				itct=sscanf(line2,"%s",nm2);
				CHECKS(itct==1,"format for product_param: rxn type [parameters]");
				CHECKS((i=stringfind(sim->mols->spname,sim->mols->nspecies,nm2))>=0,"unknown molecule in product_param");
				for(prd=0;prd<rxn->nprod && rxn->prdident[prd]!=i;prd++);
				CHECKS(prd<rxn->nprod,"molecule in product_param is not a product of this reaction");
				CHECKS(line2=strnword(line2,2),"position vector missing for product_param");
				itct=strreadnd(line2,sim->dim,postemp,NULL);
				CHECKS(itct==sim->dim,"insufficient data for position vector for product_param");
				line2=strnword(line2,sim->dim);}
			i1=RxnSetRevparam(sim,rxn,rparamt,rtemp,prd,postemp,sim->dim);
			CHECKS(i1!=1,"reversible parameter type can only be set once");
			CHECKS(i1!=2,"reversible parameter value is out of bounds");
			CHECKS(!strnword(line2,2),"unexpected text following product_param");}

		else { // unknown word
			CHECKS(0,"syntax error within reaction block: statement not recognized");}}

	CHECKS(0,"end of file encountered before end_reaction statement"); // end of file

	failure: // failure
	rxnssfree(rxnss);
	sim->rxnss[order]=NULL;
	return 1;}

/* setuprxns.  Sets up reactions from data that have already been entered.  This
sets the reaction rates, sets the reaction product placements, sets the reaction
tau values, and sets the molecule list flags.  Returns 0 for success, 1 for
failure to allocate memory, 2 for a Smoldyn bug, 3 for molecules not being set
up sufficiently, 4 for an error with setting either rates or products (in this
case, an error message is displayed to stderr), or 5 if the reaction structure
was not sufficiently set up.  This may be run at at start-up or afterwards. */
int setuprxns(simptr sim) {
	int er, order;

	for (order = 0; order < MAXORDER; order++) { // set reaction molecule lists
		er = rxnsetmollist(sim, order);
		if (er)
			return er;
	}
	er = rxnsettimestep(sim);
	if (er)
		return er + 3;

	return 0;
}

/******************************************************************************/
/************************** core simulation functions *************************/
/******************************************************************************/

/* doreact.  Executes a reaction that has already been determined to have
happened.  rxn is the reaction and mptr1 and mptr2 are the reactants, where
mptr2 is ignored for unimolecular reactions, and both are ignored for zeroth
order reactions.  ll1 is the live list of mptr1, m1 is its index in the master
list, ll2 is the live list of mptr2, and m2 is its index in the master list; if
these don�t apply (i.e. for 0th or 1st order reactions, set them to -1 and if
either m1 or m2 is unknown, again set the value to -1.  If there are multiple
molecules, they need to be in the same order as they are listed in the reaction
structure (which is only important for confspread reactions and for a completely
consistent panel destination for reactions between two surface-bound molecules).
Reactants are killed, but left in the live lists.  Any products are created on
the dead list, for transfer to the live list by the molsort routine.  Molecules
that are created are put at the reaction position, which is the average position
of the reactants weighted by the inverse of their diffusion constants, plus an
offset from the product definition.  The cluster of products is typically
rotated to a random orientation.  If the displacement was set to all 0�s
(recommended for non-reacting products), the routine is fairly fast, putting
all products at the reaction position.  If the rparamt character is RPfixed, the
orientation is fixed and there is no rotation.  Otherwise, a non-zero
displacement results in the choosing of random angles and vector rotations.  If
the system has more than three dimensions, only the first three are randomly
oriented, while higher dimensions just add the displacement to the reaction
position.  The function returns 0 for successful operation and 1 if more
molecules are required than were initially allocated.  This function lists the
correct box in the box element for each product molecule, but does not add the
product molecules to the molecule list of the box.  The bptr input is only
looked at for 0th order reactions; for these, NULL means that products should be
placed uniformly throughout the system whereas a non-NULL value means that
products should be placed uniformly throughout the listed box. */
int doreact(simptr sim, rxnptr rxn, moleculeptr mptr1, moleculeptr mptr2,
		int ll1, int m1, int ll2, int m2, double *pos, panelptr pnl) {
	int order, prd, d, nprod, dim, sf1, sf2;
	int calc;
	double dc1, dc2, x, dist;
	molssptr mols;
	moleculeptr mptr, mptrallo;
	boxptr rxnbptr;
	double v1[DIMMAX], rxnpos[DIMMAX], m3[DIMMAX * DIMMAX];
	enum MolecState ms;

	mols = sim->mols;
	dim = sim->dim;
	order = rxn->rxnss->order;

	sf1=0;
	sf2=0;
	if(mptr1 && mptr1->pnl) sf1=mptr1->pnl->srf->surface_number+1;
	if(mptr2 && mptr2->pnl) sf2=mptr2->pnl->srf->surface_number+1;
	
	
	// get reaction position in rxnpos, pnl, rxnbptr
	if (rxn->rparamt == RPconfspread) { // confspread
		pnl = NULL;
		rxnbptr = NULL;
		rxnpos[0] = 0;
	}

	else if (order == 0) { // order 0
		for (d = 0; d < dim; d++)
			rxnpos[d] = pos[d];
		rxnbptr = pos2box(sim, rxnpos);
	}

	else if (order == 1) { // order 1 
		for (d = 0; d < dim; d++)
			rxnpos[d] = mptr1->pos[d];
		pnl = mptr1->pnl;
		sf2=sf1;
		rxnbptr = mptr1->box;
	}

	else if (order == 2) { // order 2 
		dc1 = mols->difc[mptr1->ident][mptr1->mstate];
		dc2 = mols->difc[mptr2->ident][mptr2->mstate];

		//@Christine: accounting for surface specific difc 
		if (mptr1->pnl && mptr1->pnl->srf->sdifc[mptr1->ident][mptr1->mstate]>=0)
			dc1 = mptr1->pnl->srf->sdifc[mptr1->ident][mptr1->mstate];
		if (mptr2->pnl && mptr2->pnl->srf->sdifc[mptr1->ident][mptr2->mstate]>=0)
			dc2 = mptr2->pnl->srf->sdifc[mptr2->ident][mptr2->mstate];

		if (dc1 == 0 && dc2 == 0)
			x = 0.5;
		else
			x = dc2 / (dc1 + dc2);
		for (d = 0; d < dim; d++)
			rxnpos[d] = x * mptr1->pos[d] + (1.0 - x) * mptr2->pos[d];
		pnl = mptr1->pnl;
		if (!pnl && !mptr2->pnl);
		else if (!pnl && mptr2->pnl)
			pnl = mptr2->pnl;
		else if (pnl && mptr2->pnl)
			if (ptinpanel(rxnpos, mptr2->pnl, dim))
				pnl = mptr2->pnl;
		rxnbptr = mptr1->box;
	}

	else { // order > 2
		return 0;
	}

	// place products
	nprod = rxn->nprod;
	calc = 0;
	dist = 0;
	for (prd = 0; prd < nprod; prd++) {
		mptr = getnextmol(sim->mols);
		if (!mptr)
			return 1;
		mptr->ident = rxn->prdident[prd];

		if (rxn->rparamt == RPconfspread) {
			mptrallo = (prd == 0) ? mptr1 : mptr2;
			mptr->mstate = rxn->prdstate[prd];
			mptr->box = mptrallo->box;
			for (d = 0; d < dim; d++)
				mptr->pos[d] = mptr->posx[d] = mptrallo->pos[d];
			mptr->pnl = (mptr->mstate == MSsoln) ? NULL : mptrallo->pnl;
		}

		else if (rxn->rparamt == RPbounce) {
			mptr->mstate = rxn->prdstate[prd];
			mptrallo = (prd == 0) ? mptr1 : mptr2;
			mptr->box = mptrallo->box;
			for (d = 0; d < dim; d++)
				mptr->posx[d] = mptrallo->pos[d];
			if (prd == 0) {
				dist = 0;
				for (d = 0; d < dim; d++)
					dist += (mptr2->pos[d] - mptr1->pos[d]) * (mptr2->pos[d]
							- mptr1->pos[d]);
				dist = sqrt(dist);
			}
			for (d = 0; d < dim; d++)
				mptr->pos[d] = rxnpos[d] - (mptr2->pos[d] - mptr1->pos[d])
						/ dist * rxn->prdpos[sf1][sf2][prd][0];
			mptr->pnl = (mptr->mstate == MSsoln) ? NULL : mptrallo->pnl;
		}

		else {
			mptr->box = rxnbptr;
			for (d = 0; d < dim; d++)
				mptr->posx[d] = rxnpos[d];

			mptr->mstate = ms = rxn->prdstate[prd];
			mptr->pnl = pnl;
			if (!pnl)
				; // soln -> soln
			else if (ms == MSsoln) { // surf -> front soln
				mptr->pnl = NULL;
				fixpt2panel(mptr->posx, pnl, dim, PFfront, sim->srfss->epsilon);
			}
			else if (ms == MSbsoln) { // surf -> back soln
				mptr->mstate = MSsoln;
				mptr->pnl = NULL;
				fixpt2panel(mptr->posx, pnl, dim, PFback, sim->srfss->epsilon);
			}
			else if (ms == MSfront) { // surf -> front surf
				fixpt2panel(mptr->posx, pnl, dim, PFfront, sim->srfss->epsilon);
			}
			else if (ms == MSback) { // surf -> back surf
				fixpt2panel(mptr->posx, pnl, dim, PFback, sim->srfss->epsilon);
			}
			else { // surf -> surf: up, down
				fixpt2panel(mptr->posx, pnl, dim, PFnone, sim->srfss->epsilon);
			}

			for (d = 0; d < dim && rxn->prdpos[sf1][sf2][prd][d] == 0; d++);
			if (d != dim) {
				if (rxn->rparamt == RPfixed) {
					for (d = 0; d < dim; d++)
						v1[d] = rxn->prdpos[sf1][sf2][prd][d];
				}
				else if (dim == 1) {
					if (!calc) {
						m3[0] = signrand();
						calc = 1;
					}
					v1[0] = m3[0] * rxn->prdpos[sf1][sf2][prd][0];
				}
				else if (dim == 2) {
					if (!calc) {
						DirCosM2D(m3, unirandCOD(0, 2 * PI));
						calc = 1;
					}
					dotMVD(m3, rxn->prdpos[sf1][sf2][prd], v1, 2, 2);
				}
				else if (dim == 3) { 
					if (!calc) {
						DirCosMD(m3, thetarandCCD(), unirandCOD(0, 2 * PI),
								unirandCOD(0, 2 * PI));
						calc = 1;
					}
					dotMVD(m3, rxn->prdpos[sf1][sf2][prd], v1, 3, 3);
				}
				else {
					if (!calc) {
						DirCosMD(m3, thetarandCCD(), unirandCOD(0, 2 * PI),
								unirandCOD(0, 2 * PI));
						calc = 1;
					}
					dotMVD(m3, rxn->prdpos[sf1][sf2][prd], v1, 3, 3);
					for (d = 3; d < dim; d++)
						v1[d] = rxn->prdpos[sf1][sf2][prd][d];
				}
				for (d = 0; d < dim; d++)
					mptr->pos[d] = mptr->posx[d] + v1[d];
			}
			else {
				for (d = 0; d < dim; d++)
					mptr->pos[d] = mptr->posx[d];
			}
		}

		mptr->list = sim->mols->listlookup[mptr->ident][mptr->mstate];
		if (sim->mols->expand[mptr->ident]) { //???????? new code
			mzrExpandSpecies(sim, mptr->ident);
		}
	} //??????? new code

	if (mptr1)
		molkill(sim, mptr1, ll1, m1); // kill reactants
	if (mptr2)
		molkill(sim, mptr2, ll2, m2);

	return 0;
}

/* zeroreact.  Figures out how many molecules to create for each zeroth order
reaction and then tells doreact to create them.  It returns 0 for success or 1
if not enough molecules were allocated initially. */
int zeroreact(simptr sim) {
	int i, r, nmol;
	rxnptr rxn;
	rxnssptr rxnss;
	double pos[DIMMAX];
	panelptr pnl;

	pnl = NULL;
	rxnss = sim->rxnss[0];
	if (!rxnss)
		return 0;
	for (r = 0; r < rxnss->totrxn; r++) {
		rxn = rxnss->rxn[r];
		nmol = poisrandD(rxn->prob[0][0]);
		if(rxn->srf) {
		      nmol = poisrandD(rxn->prob[rxn->srf->surface_number][rxn->srf->surface_number]);
		}
		for (i = 0; i < nmol; i++) {
			if (rxn->cmpt)
				compartrandpos(sim, pos, rxn->cmpt);
			else if (rxn->srf)
				pnl = surfrandpos(rxn->srf, pos, sim->dim);
			else
				systemrandpos(sim, pos);
			if (doreact(sim, rxn, NULL, NULL, -1, -1, -1, -1, pos, pnl))
				return 1;
		}
		sim->eventcount[ETrxn0] += nmol;
	}
	return 0;
}

/* unireact.  Identifies and performs all unimolecular reactions.  Reactions
that should occur are sent to doreact to process them.  The function returns 0
for success or 1 if not enough molecules were allocated initially. */
int unireact(simptr sim) {
	rxnssptr rxnss;
	rxnptr rxn, *rxnlist;
	moleculeptr *mlist, mptr;
	int *nrxn, **table;
	int i, j, m, nmol, ll, mptr_sf;
	enum MolecState ms;

	rxnss = sim->rxnss[1];
	if (!rxnss)
		return 0;
	nrxn = rxnss->nrxn;
	table = rxnss->table;
	rxnlist = rxnss->rxn;
	for (ll = 0; ll < sim->mols->nlist; ll++)
		if (rxnss->rxnmollist[ll]) {
			mlist = sim->mols->live[ll];
			nmol = sim->mols->nl[ll];
			for (m = 0; m < nmol; m++) {
				mptr = mlist[m];
				i = mptr->ident;
				ms = mptr->mstate;
				mptr_sf=0;
				if(mptr->pnl) mptr_sf=mptr->pnl->srf->surface_number;
				for (j = 0; j < nrxn[i]; j++) {
					rxn = rxnlist[table[i][j]];
					if ((!rxn->cmpt && !rxn->srf) || (rxn->cmpt
							&& posincompart(sim, mptr->pos, rxn->cmpt))
							|| (rxn->srf && mptr->pnl && mptr->pnl->srf
									== rxn->srf))
						if (coinrandD(rxn->prob[mptr_sf][mptr_sf]) && rxn->permit[ms]
								&& mptr->ident != 0) {
							if (doreact(sim, rxn, mptr, NULL, ll, m, -1, -1,
									NULL, NULL))
								return 1;
							sim->eventcount[ETrxn1]++;
							j = nrxn[i];
						}
				}
			}
		}
	return 0;
}

/* morebireact.  Given a probable reaction from bireact, this orders the
reactants, checks for reaction permission, moves a reactant in case of periodic
boundaries, increments the appropriate event counter, and calls doreact to
perform the reaction.  The return value is 0 for success (which may include no
reaction) and 1 for failure. */
int morebireact(simptr sim, rxnptr rxn, moleculeptr mptr1, moleculeptr mptr2,
		int ll1, int m1, int ll2, enum EventType et) {
	moleculeptr mptrA, mptrB;
	int d, swap;
	enum MolecState ms, msA, msB;

	if (rxn->cmpt && !(posincompart(sim, mptr1->pos, rxn->cmpt)
			&& posincompart(sim, mptr2->pos, rxn->cmpt)))
		return 0;
	if (rxn->srf && !((mptr1->pnl && mptr1->pnl->srf == rxn->srf)
			|| (mptr2->pnl && mptr2->pnl->srf == rxn->srf)))
		return 0;

	if (mptr1->ident == rxn->rctident[0]) {
		mptrA = mptr1;
		mptrB = mptr2;
		swap = 0;
	}
	else {
		mptrA = mptr2;
		mptrB = mptr1;
		swap = 1;
	}

	msA = mptrA->mstate;
	msB = mptrB->mstate;
	if (msA == MSsoln && msB != MSsoln)
		msA
				= (panelside(mptrA->pos, mptrB->pnl, sim->dim, NULL) == PFfront) ? MSsoln
						: MSbsoln;
	else if (msB == MSsoln && msA != MSsoln)
		msB
				= (panelside(mptrB->pos, mptrA->pnl, sim->dim, NULL) == PFfront) ? MSsoln
						: MSbsoln;
	ms = msA * MSMAX1 + msB;

	if (rxn->permit[ms]) {
		if (et == ETrxn2wrap && rxn->rparamt != RPconfspread) {

			//Christine
			double difcA = sim->mols->difc[mptrA->ident][mptrA->mstate];
			double difcB = sim->mols->difc[mptrB->ident][mptrB->mstate];
			if (mptrA->pnl
					&& mptrA->pnl->srf->sdifc[mptrA->ident][mptrA->mstate]>=0)
				difcA = mptrA->pnl->srf->sdifc[mptrA->ident][mptrA->mstate];
			if (mptrB->pnl
					&& mptrB->pnl->srf->sdifc[mptrB->ident][mptrB->mstate]>=0)
				difcB = mptrB->pnl->srf->sdifc[mptrB->ident][mptrB->mstate];

			if (difcA < difcB) //Christine: If cond. changed to shorter version due to above def.
				for (d = 0; d < sim->dim; d++)
					mptrB->pos[d] = mptrA->pos[d];
			else
				for (d = 0; d < sim->dim; d++)
					mptrA->pos[d] = mptrB->pos[d];
		}

		sim->eventcount[et]++;
		if (!swap)
			return doreact(sim, rxn, mptrA, mptrB, ll1, m1, ll2, -1, NULL, NULL);
		else
			return doreact(sim, rxn, mptrA, mptrB, ll2, -1, ll1, m1, NULL, NULL);
	}

	return 0;
}

/* bireact.  Identifies likely bimolecular reactions, sending ones that probably
occur to morebireact for permission testing and reacting.  neigh tells the
routine whether to consider only reactions between neighboring boxes (neigh=1)
or only reactions within a box (neigh=0).  The former are relatively slow and so
can be ignored for qualitative simulations by choosing a lower simulation
accuracy value.  In cases where walls are periodic, it is possible to have
reactions over the system walls.  The function returns 0 for success or 1 if not
enough molecules were allocated initially. */
int bireact(simptr sim, int neigh) {
	int surf_num1, surf_num2, dim, maxspecies, ll1, ll2, i, j, d, *nl, nmol2,
			b2, m1, m2, bmax, wpcode, nlist, maxlist;
	int *nrxn, **table;
	double dist2, pos2;
	rxnssptr rxnss;
	rxnptr rxn, *rxnlist;
	boxptr bptr;
	moleculeptr **live, *mlist2, mptr1, mptr2;

	rxnss = sim->rxnss[2];
	if (!rxnss)
		return 0;
	dim = sim->dim;
	live = sim->mols->live;
	maxspecies = rxnss->maxspecies;
	maxlist = rxnss->maxlist;
	nlist = sim->mols->nlist;
	nrxn = rxnss->nrxn;
	table = rxnss->table;
	rxnlist = rxnss->rxn;
	nl = sim->mols->nl;

	if (!neigh) { // same box
		for (ll1 = 0; ll1 < nlist; ll1++)
			for (ll2 = ll1; ll2 < nlist; ll2++)
				if (rxnss->rxnmollist[ll1 * maxlist + ll2])
					for (m1 = 0; m1 < nl[ll1]; m1++) {
						mptr1 = live[ll1][m1];
						bptr = mptr1->box;
						mlist2 = bptr->mol[ll2];
						nmol2 = bptr->nmol[ll2];
						for (m2 = 0; m2 < nmol2 && mlist2[m2] != mptr1; m2++) {
							mptr2 = mlist2[m2];
							i = mptr1->ident * maxspecies + mptr2->ident;
							for (j = 0; j < nrxn[i]; j++) {
								rxn = rxnlist[table[i][j]];
								dist2 = 0;
								for (d = 0; d < dim; d++)
									dist2 += (mptr1->pos[d] - mptr2->pos[d])
											* (mptr1->pos[d] - mptr2->pos[d]);
								//Christine Have to check whether a molecule is on a surface or not and then apply correct bindrad (definitely have to change that for accounting for mol on different surfaces...)
								surf_num1 = 0; //cause 0 is default: no surface in the lists
								surf_num2 = 0;
								if (mptr1->pnl)
									surf_num1 = mptr1->pnl->srf->surface_number+1;
								if (mptr2->pnl)
									surf_num2 = mptr2->pnl->srf->surface_number+1; 
								if (dist2
										<= rxn->bindrad2[surf_num1][surf_num2]
										&& (rxn->prob[surf_num1][surf_num2] == 1 || randCOD()
												< rxn->prob[surf_num1][surf_num2]) && (mptr1->mstate
										!= MSsoln || mptr2->mstate != MSsoln
										|| !rxnXsurface(sim, mptr1, mptr2))
										&& mptr1->ident != 0 && mptr2->ident
										!= 0) {
									if (morebireact(sim, rxn, mptr1, mptr2,
											ll1, m1, ll2, ETrxn2intra))
										return 1;
									if (mptr1->ident == 0) {
										j = nrxn[i];
										m2 = nmol2;
									}
								}
							}
						}
					}
	}

	else { // neighbor box
		for (ll1 = 0; ll1 < nlist; ll1++)
			for (ll2 = ll1; ll2 < nlist; ll2++)
				if (rxnss->rxnmollist[ll1 * maxlist + ll2])
					for (m1 = 0; m1 < nl[ll1]; m1++) {
						mptr1 = live[ll1][m1];
						bptr = mptr1->box;
						bmax = (ll1 != ll2) ? bptr->nneigh : bptr->midneigh;
						for (b2 = 0; b2 < bmax; b2++) {
							mlist2 = bptr->neigh[b2]->mol[ll2];
							nmol2 = bptr->neigh[b2]->nmol[ll2];
							if (bptr->wpneigh && bptr->wpneigh[b2]) { // neighbor box with wrapping
								wpcode = bptr->wpneigh[b2];
								for (m2 = 0; m2 < nmol2; m2++) {
									mptr2 = mlist2[m2];
									i = mptr1->ident * maxspecies
											+ mptr2->ident;
									for (j = 0; j < nrxn[i]; j++) {
										rxn = rxnlist[table[i][j]];
										dist2 = 0;
										for (d = 0; d < dim; d++) {
											if ((wpcode >> 2 * d & 3) == 0)
												dist2
														+= (mptr1->pos[d]
																- mptr2->pos[d])
																* (mptr1->pos[d]
																		- mptr2->pos[d]);
											else if ((wpcode >> 2 * d & 3) == 1) {
												pos2
														= sim->wlist[2 * d + 1]->pos
																- sim->wlist[2
																		* d]->pos;
												dist2 += (mptr1->pos[d]
														- mptr2->pos[d] + pos2)
														* (mptr1->pos[d]
																- mptr2->pos[d]
																+ pos2);
											}
											else {
												pos2
														= sim->wlist[2 * d + 1]->pos
																- sim->wlist[2
																		* d]->pos;
												dist2 += (mptr1->pos[d]
														- mptr2->pos[d] - pos2)
														* (mptr1->pos[d]
																- mptr2->pos[d]
																- pos2);
											}
										}
										//Christine: Have to check whether a molecule is on a surface or not and then apply correct bindrad (definitely have to change that for accounting for mol on different surfaces...)
										surf_num1 = 0;
										if (mptr1->pnl)
											surf_num1
													= mptr1->pnl->srf->surface_number+1;

										surf_num2 = 0;
										if (mptr2->pnl)
											surf_num2
													= mptr2->pnl->srf->surface_number+1;
										if (dist2
												<= rxn->bindrad2[surf_num1][surf_num2]
												&& (rxn->prob[surf_num1][surf_num2] == 1 || randCOD()
														< rxn->prob[surf_num1][surf_num2])
												&& mptr1->ident != 0
												&& mptr2->ident != 0) {
											if (morebireact(sim, rxn, mptr1,
													mptr2, ll1, m1, ll2,
													ETrxn2wrap))
												return 1;
											if (mptr1->ident == 0) {
												j = nrxn[i];
												m2 = nmol2;
												b2 = bmax;
											}
										}
									}
								}
							}

							else
								// neighbor box, no wrapping
								for (m2 = 0; m2 < nmol2; m2++) {
									mptr2 = mlist2[m2];
									i = mptr1->ident * maxspecies
											+ mptr2->ident;
									for (j = 0; j < nrxn[i]; j++) {
										rxn = rxnlist[table[i][j]];
										dist2 = 0;
										for (d = 0; d < dim; d++)
											dist2 += (mptr1->pos[d]
													- mptr2->pos[d])
													* (mptr1->pos[d]
															- mptr2->pos[d]);
										//Christine Have to check whether a molecule is on a surface or not and then apply correct bindrad (definitely have to change that for accounting for mol on different surfaces...)
										surf_num1 = 0;
										surf_num2 = 0;
										if (mptr1->pnl)
											surf_num1
													= mptr1->pnl->srf->surface_number+1;
										if (mptr2->pnl)
											surf_num2
													= mptr2->pnl->srf->surface_number+1;
										if (dist2
												<= rxn->bindrad2[surf_num1][surf_num2]
												&& (rxn->prob[surf_num1][surf_num2] == 1 || randCOD()
														< rxn->prob[surf_num1][surf_num2])
												&& (mptr1->mstate != MSsoln
														|| mptr2->mstate
																!= MSsoln
														|| !rxnXsurface(sim,
																mptr1, mptr2))
												&& mptr1->ident != 0
												&& mptr2->ident != 0) {
											if (morebireact(sim, rxn, mptr1,
													mptr2, ll1, m1, ll2,
													ETrxn2inter))
												return 1;
											if (mptr1->ident == 0) {
												j = nrxn[i];
												m2 = nmol2;
												b2 = bmax;
											}
										}
									}
								}
						}
					}
	}

	return 0;
}

//??????? start of threading code


/* unireact_threaded */
int unireact_threaded(simptr sim) {
#ifndef THREADING
	return 2;
#else
	rxnssptr rxnss;
	//	rxnptr rxn,*rxnlist;
	rxnptr *rxnlist;
	//	moleculeptr *mlist,mptr;
	int *nrxn,**table;
	//	int i,j,m,nmol,ll;
	int ll;
	//	enum MolecState ms;
	int nthreads = sim->threads->nthreads;
	stack* current_thread_input_stack;

	PARAMS_unireact_threaded_calculate_reactions theParams;
	theParams.sim = sim;

	rxnss=sim->rxnss[1];
	if(!rxnss) return 0;
	nrxn=rxnss->nrxn;
	table=rxnss->table;
	rxnlist=rxnss->rxn;

	for(ll=0;ll<sim->mols->nlist;ll++)
	if(rxnss->rxnmollist[ll])
	{
		theParams.ll = ll;

		int current_ndx = 0;
		int total_num_to_process = sim->mols->nl[ll];
		int stride = calculatestride(total_num_to_process, nthreads);

		//
		// Create nthreads
		//
		// Create threads 0... number_threads - 2.
		int thread_ndx;
		for(thread_ndx = 0; thread_ndx != nthreads - 1; ++thread_ndx)
		{
			current_thread_input_stack = sim->threads->thread[thread_ndx]->input_stack;

			clearthreaddata( sim->threads->thread[thread_ndx]);

			theParams.mol_ndx1 = current_ndx;
			theParams.mol_ndx2 = current_ndx += stride;
			theParams.output_stack = sim->threads->thread[thread_ndx]->output_stack;

			push_data_onto_stack( current_thread_input_stack, &theParams, sizeof(theParams)); // this copies over the inputParams data, so the fact that it is used to seed multiple threads is no problem.
			pthread_create(sim->threads->thread[thread_ndx]->thread_id, NULL, unireact_threaded_calculate_reactions, (void*) current_thread_input_stack->stack_data);
		}
		{ // Create the last thread
			clearthreaddata( sim->threads->thread[nthreads - 1]);
			current_thread_input_stack = sim->threads->thread[nthreads-1]->input_stack;
			theParams.mol_ndx1 = current_ndx;
			theParams.mol_ndx2 = total_num_to_process;
			theParams.output_stack = sim->threads->thread[nthreads-1]->output_stack;
			push_data_onto_stack( current_thread_input_stack, &theParams, sizeof(theParams)); // this copies over the inputParams data, so the fact that it is used to seed multiple threads is no problem.
			pthread_create(sim->threads->thread[nthreads-1]->thread_id, NULL, unireact_threaded_calculate_reactions, (void*) current_thread_input_stack->stack_data);
		}

		// Join thread and process data.
		for(thread_ndx = 0; thread_ndx != nthreads; ++thread_ndx)
		{
			pthread_join( *((pthread_t*) sim->threads->thread[thread_ndx]->thread_id), NULL);

			void* current_thread_data = sim->threads->thread[ thread_ndx ]->output_stack->stack_data;
			int number_to_process = *( (int*) current_thread_data);

			PARAMS_doreact* doreact_param_array = (PARAMS_doreact*) ((int*) current_thread_data + 1);

			int proposed_rxn_ndx;
			for(proposed_rxn_ndx = 0; proposed_rxn_ndx != number_to_process; ++proposed_rxn_ndx)
			{
				simptr sim = doreact_param_array[proposed_rxn_ndx].sim;
				rxnptr rxn = doreact_param_array[proposed_rxn_ndx].rxn;
				moleculeptr mptr1 = doreact_param_array[proposed_rxn_ndx].mptr1;
				moleculeptr mptr2 = doreact_param_array[proposed_rxn_ndx].mptr2;
				int ll1 = doreact_param_array[proposed_rxn_ndx].ll1;
				int m1 = doreact_param_array[proposed_rxn_ndx].m1;
				int ll2 = doreact_param_array[proposed_rxn_ndx].ll2;
				int m2 = doreact_param_array[proposed_rxn_ndx].m2;
				double *pos = doreact_param_array[proposed_rxn_ndx].pos;
				panelptr pnl = doreact_param_array[proposed_rxn_ndx].pnl;

				if(mptr1 && mptr1->ident)
				{
					if( doreact(sim, rxn, mptr1, mptr2, ll1, m1, ll2, m2, pos, pnl) ) return 1;
					sim->eventcount[ETrxn1]++;
				}}}}

	return 0;
#endif
}

void*
unireact_threaded_calculate_reactions(void* data) {
#ifndef THREADING
	return NULL;
#else
	PARAMS_unireact_threaded_calculate_reactions* pParams = (PARAMS_unireact_threaded_calculate_reactions*) data;

	simptr sim = pParams->sim;
	int ll = pParams->ll;
	int mol_ndx1 = pParams->mol_ndx1;
	int mol_ndx2 = pParams->mol_ndx2;
	stack* output_stack = pParams->output_stack;

	rxnssptr rxnss;
	rxnptr rxn,*rxnlist;
	moleculeptr *mlist,mptr;
	int *nrxn,**table;
	int i,j,m,nmol;
	enum MolecState ms;

	rxnss=sim->rxnss[1];
	if(!rxnss) return 0;
	nrxn=rxnss->nrxn;
	table=rxnss->table;
	rxnlist=rxnss->rxn;
	mlist=sim->mols->live[ll];
	nmol=sim->mols->nl[ll];

	PARAMS_doreact outputParams;
	outputParams.sim = sim;
	outputParams.mptr2 = NULL;
	outputParams.ll2 = -1;
	outputParams.m2 = -1;
	outputParams.pos = NULL;
	outputParams.pnl = NULL;

	int num_found = 0;
	push_data_onto_stack(output_stack, &num_found, sizeof(num_found));

	for( m = mol_ndx1; m != mol_ndx2; ++m)
	{
		mptr=mlist[m];
		i=mptr->ident;
		ms=mptr->mstate;

		for(j=0;j<nrxn[i];j++)
		{
			rxn=rxnlist[table[i][j]];
			if((!rxn->cmpt && !rxn->srf) || (rxn->cmpt && posincompart(sim,mptr->pos,rxn->cmpt)) || (rxn->srf && mptr->pnl && mptr->pnl->srf==rxn->srf))
			if(coinrandD(rxn->prob[0][0]) && rxn->permit[ms] && mptr->ident!=0)
			{
				outputParams.rxn = rxn;
				outputParams.mptr1 = mptr;
				outputParams.ll1 = ll;
				outputParams.m1 = m;

				push_data_onto_stack( output_stack, &outputParams, sizeof(outputParams));
				*((int*) output_stack->stack_data) += 1;
			}
		}
	}
	return NULL;
#endif
}

/* bireact_threaded */
int bireact_threaded(simptr sim, int neigh) {
#ifndef THREADING
	return 2;
#else

	if (!neigh)
	bireact_threaded_intrabox(sim);
	else bireact_threaded_interbox(sim);
	return 0;

#endif
}

int bireact_threaded_intrabox(simptr sim) {
#ifndef THREADING
	return 2;
#else
	//	int dim,maxspecies,ll1,ll2,i,j,d,*nl,nmol2,b2,m1,m2,bmax,wpcode,nlist,maxlist,nthreads;
	int dim,maxspecies,ll1,ll2,*nl,nlist,maxlist,nthreads;
	int *nrxn,**table;
	//	double dist2,pos2;
	rxnssptr rxnss;
	//	rxnptr rxn,*rxnlist;
	rxnptr *rxnlist;
	//	boxptr bptr;
	//	moleculeptr **live,*mlist2,mptr1,mptr2;
	moleculeptr **live;

	rxnss=sim->rxnss[2];
	if(!rxnss) return 0;
	dim=sim->dim;
	live=sim->mols->live;
	maxspecies=rxnss->maxspecies;
	maxlist=rxnss->maxlist;
	nlist=sim->mols->nlist;
	nrxn=rxnss->nrxn;
	table=rxnss->table;
	rxnlist=rxnss->rxn;
	nl=sim->mols->nl;
	nthreads = sim->threads->nthreads;
	stack* current_thread_input_stack;

	for(ll1=0; ll1 < nlist; ll1++) {
		for(ll2=ll1; ll2 < nlist; ll2++) {
			if(rxnss->rxnmollist[ll1 * maxlist + ll2])
			{
				int total_num_to_process = nl[ll1];
				int per_thread_to_process = (total_num_to_process + (nthreads - (total_num_to_process % nthreads))) / nthreads; // This equals ceil( total_num_to_process / nthreads).

				int initial_ndx = 0;
				PARAMS_check_for_intrabox inputParams;
				inputParams.sim = sim;
				inputParams.ll_ndx_1 = ll1;
				inputParams.ll_ndx_2 = ll2;

				// Create all the threads. Note that the creation of the final thread is it's own thing, becuase
				// it will likely have an odd number of molecules to process.

				int thread_ndx;
				for(thread_ndx = 0; thread_ndx != nthreads - 1; ++thread_ndx)
				{
					// Clear this thread's dedicated input and output stacks...
					clearthreaddata( sim->threads->thread[thread_ndx]);
					current_thread_input_stack = sim->threads->thread[thread_ndx]->input_stack;

					inputParams.first_ndx = initial_ndx;
					inputParams.second_ndx = initial_ndx += per_thread_to_process;
					inputParams.output_stack = sim->threads->thread[thread_ndx]->output_stack;

					push_data_onto_stack( current_thread_input_stack, &inputParams, sizeof(inputParams)); // this copies over the inputParams data, so the fact that it is used to seed multiple threads is no problem.

					pthread_create( (pthread_t*) sim->threads->thread[thread_ndx]->thread_id, NULL, check_for_intrabox_bireactions_threaded, (void*) current_thread_input_stack->stack_data);

				}
				{ // Process the last index seperately, because it will likely have an odd number of molecules to process....

					clearthreaddata( sim->threads->thread[ nthreads - 1] );
					current_thread_input_stack = sim->threads->thread[ nthreads - 1]->input_stack;

					inputParams.first_ndx = initial_ndx;
					inputParams.second_ndx = total_num_to_process;
					inputParams.output_stack = sim->threads->thread[nthreads - 1]->output_stack;

					push_data_onto_stack( current_thread_input_stack, &inputParams, sizeof(inputParams)); // this copies over the inputParams data, so the fact that it is used to seed multiple threads is no problem.

					pthread_create((pthread_t*) sim->threads->thread[nthreads - 1]->thread_id, NULL, check_for_intrabox_bireactions_threaded, (void*) current_thread_input_stack->stack_data);
				}

				// Join all the threads.
				for( thread_ndx = 0; thread_ndx != nthreads; ++thread_ndx)
				pthread_join( *((pthread_t*) sim->threads->thread[thread_ndx]->thread_id), NULL);

				int number_to_process, paramNdx;;
				PARAMS_morebireact* morebireact_param_array;

				// Permaybehaps this for loop can be combined with the previous one, with perhaps slight savings of time.  However, to prevent errors, for now I am leaving them seperate.
				for( thread_ndx = 0; thread_ndx != nthreads; ++thread_ndx)
				{
					void* current_thread_data = sim->threads->thread[ thread_ndx ]->output_stack->stack_data;

					number_to_process = *( (int*) current_thread_data);
					morebireact_param_array = (PARAMS_morebireact*) ((int*) current_thread_data + 1);

					for(paramNdx = 0; paramNdx != number_to_process; ++paramNdx)
					{
						rxnptr rxn = morebireact_param_array[paramNdx].rxn_to_execute;
						moleculeptr mptr1 = morebireact_param_array[paramNdx].mol_ptr_1;
						moleculeptr mptr2 = morebireact_param_array[paramNdx].mol_ptr_2;
						int ll1 = morebireact_param_array[paramNdx].ll_ndx_1;
						int m1 = morebireact_param_array[paramNdx].mol_ndx_1;
						int ll2 = morebireact_param_array[paramNdx].ll_ndx_2;

						if( mptr1->ident && mptr2->ident) // Make sure they are both legit...
						{
							if(morebireact(sim,rxn,mptr1,mptr2,ll1,m1,ll2,ETrxn2intra)) return 1;

						}
					}

				}
			}
		}
	}

	return 0;
#endif
}

int bireact_threaded_interbox(simptr sim) {
#ifndef THREADING
	return 2;
#else
	//	int dim,maxspecies,ll1,ll2,i,j,d,*nl,nmol2,b2,m1,m2,bmax,wpcode,nlist,maxlist, nthreads;
	int dim,maxspecies,ll1,ll2,*nl,nlist,maxlist, nthreads;
	int *nrxn,**table;
	//	double dist2,pos2;
	rxnssptr rxnss;
	//	rxnptr rxn,*rxnlist;
	rxnptr *rxnlist;
	//	boxptr bptr;
	//	moleculeptr **live,*mlist2,mptr1,mptr2;
	moleculeptr **live;

	rxnss=sim->rxnss[2];
	if(!rxnss) return 0;
	dim=sim->dim;
	live=sim->mols->live;
	maxspecies=rxnss->maxspecies;
	maxlist=rxnss->maxlist;
	nlist=sim->mols->nlist;
	nrxn=rxnss->nrxn;
	table=rxnss->table;
	rxnlist=rxnss->rxn;
	nl=sim->mols->nl;
	nthreads=sim->threads->nthreads;

	// BEGIN COMPUTATION

	for(ll1=0;ll1<nlist;ll1++) {
		for(ll2=ll1;ll2<nlist;ll2++) {

			if(rxnss->rxnmollist[ll1*maxlist+ll2])
			{
				int total_num_to_process = nl[ll1];
				int per_thread_to_process = (total_num_to_process + (nthreads - (total_num_to_process % nthreads))) / nthreads; // This equals ceil( total_num_to_process / nthreads).

				stack* current_thread_input_stack;

				int initial_ndx = 0;

				PARAMS_check_for_intrabox inputParams;
				inputParams.sim = sim;
				inputParams.ll_ndx_1 = ll1;
				inputParams.ll_ndx_2 = ll2;

				//
				// Create the threads.
				//
				//
				int thread_ndx;
				for(thread_ndx = 0; thread_ndx != nthreads - 1; ++thread_ndx)
				{
					// Clear this thread's dedicated input and output stacks...
					clearthreaddata( sim->threads->thread[thread_ndx]);
					current_thread_input_stack = sim->threads->thread[thread_ndx]->input_stack;

					inputParams.first_ndx = initial_ndx;
					inputParams.second_ndx = initial_ndx += per_thread_to_process;
					inputParams.output_stack = sim->threads->thread[thread_ndx]->output_stack;

					push_data_onto_stack( current_thread_input_stack, &inputParams, sizeof(inputParams)); // this copies over the inputParams data, so the fact that it is used to seed multiple threads is no problem.
					pthread_create(sim->threads->thread[thread_ndx]->thread_id, NULL, check_for_interbox_bireactions_threaded, (void*) current_thread_input_stack->stack_data);

				}
				{ // Process the last index seperately, because it will have an odd number of molecules to process....

					clearthreaddata( sim->threads->thread[nthreads - 1] );
					current_thread_input_stack = sim->threads->thread[nthreads - 1]->input_stack;

					inputParams.first_ndx = initial_ndx;
					inputParams.second_ndx = total_num_to_process;
					inputParams.output_stack = sim->threads->thread[nthreads - 1]->output_stack;

					push_data_onto_stack( current_thread_input_stack, &inputParams, sizeof(inputParams)); // this copies over the inputParams data, so the fact that it is used to seed multiple threads is no problem.
					pthread_create(sim->threads->thread[nthreads - 1]->thread_id, NULL, check_for_interbox_bireactions_threaded, (void*) current_thread_input_stack->stack_data);
				}

				// Join all the threads.
				for( thread_ndx = 0; thread_ndx != nthreads; ++thread_ndx)
				pthread_join( *((pthread_t*)sim->threads->thread[thread_ndx]->thread_id), NULL);

				// Permaybehaps this for loop can be combined with the previous one, with perhaps slight savings of time.  However, to prevent errors, for now I am leaving them seperate.
				for( thread_ndx = 0; thread_ndx != nthreads; ++thread_ndx)
				{
					// The first value in the stack is an int that says how many
					// rxn, mptr1, mptr2, ll1, m1, ll2's there are...

					void* current_thread_data = sim->threads->thread[ thread_ndx ]->output_stack->stack_data;
					int number_to_process = *( (int*) current_thread_data);

					PARAMS_morebireact* morebireact_param_array = (PARAMS_morebireact*) ((int*) current_thread_data + 1);

					int paramNdx;
					for(paramNdx = 0; paramNdx != number_to_process; ++paramNdx)
					{

						rxnptr rxn = morebireact_param_array[paramNdx].rxn_to_execute;
						moleculeptr mptr1 = morebireact_param_array[paramNdx].mol_ptr_1;
						moleculeptr mptr2 = morebireact_param_array[paramNdx].mol_ptr_2;
						int ll1 = morebireact_param_array[paramNdx].ll_ndx_1;
						int m1 = morebireact_param_array[paramNdx].mol_ndx_1;
						int ll2 = morebireact_param_array[paramNdx].ll_ndx_2;
						enum EventType et = morebireact_param_array[paramNdx].et;

						// It is probably enough to just check for one of these...
						if( mptr1->ident && mptr2->ident)
						{
							// VDEBUG_printf("X morebireact: (%p, %p, %p, %d, %d, %d)\n", rxn, mptr1, mptr2, ll1, m1, ll2);
							// printf("X morebireact: (%p, %p, %p, %d, %d, %d)\n", rxn, mptr1, mptr2, ll1, m1, ll2);
							if(morebireact(sim,rxn,mptr1,mptr2,ll1,m1,ll2,et)) return 1;
						}
					}
				}
			}
		}
	}

	return 0;
#endif
}

// We pass in the stack data, NOT the stack itself.
void*
check_for_interbox_bireactions_threaded(void* data) {
#ifndef THREADING
	return NULL;
#else
	PARAMS_check_for_intrabox* interbox_input_params = (PARAMS_check_for_intrabox*) data;

	simptr sim = interbox_input_params->sim;
	int live_list_ndx_1 = interbox_input_params->ll_ndx_1;
	int live_list_ndx_2 = interbox_input_params->ll_ndx_2;
	int first_ndx = interbox_input_params->first_ndx;
	int second_ndx = interbox_input_params->second_ndx;;
	stack* output_stack = interbox_input_params->output_stack;

	int num_found = 0;
	push_data_onto_stack(output_stack, &num_found, sizeof(num_found));

	PARAMS_morebireact morebireact_params;

	////// The inner loop....

	int surf_num1, surf_num2, dim,maxspecies,ll1,ll2,i,j,d,*nl,nmol2,b2,m1,m2,bmax,wpcode,nlist,maxlist;
	int *nrxn,**table;
	double dist2,pos2;
	rxnssptr rxnss;
	rxnptr rxn,*rxnlist;
	boxptr bptr;
	moleculeptr **live,*mlist2,mptr1,mptr2;

	ll1 = live_list_ndx_1;
	ll2 = live_list_ndx_2;

	rxnss=sim->rxnss[2];
	dim=sim->dim;
	live=sim->mols->live;
	maxspecies=rxnss->maxspecies;
	maxlist=rxnss->maxlist;
	nlist=sim->mols->nlist;
	nrxn=rxnss->nrxn;
	table=rxnss->table;
	rxnlist=rxnss->rxn;
	nl=sim->mols->nl;

	for(m1=first_ndx;m1!=second_ndx;m1++)
	{
		mptr1=live[ll1][m1];
		bptr=mptr1->box;
		bmax=(ll1!=ll2)?bptr->nneigh:bptr->midneigh;
		for(b2=0;b2<bmax;b2++)
		{
			mlist2=bptr->neigh[b2]->mol[ll2];
			nmol2=bptr->neigh[b2]->nmol[ll2];

			if(bptr->wpneigh && bptr->wpneigh[b2]) // neighbor box with wrapping
			{
				wpcode=bptr->wpneigh[b2];
				for(m2=0;m2<nmol2;m2++)
				{
					mptr2=mlist2[m2];
					i=mptr1->ident*maxspecies+mptr2->ident;
					for(j=0;j<nrxn[i];j++)
					{
						rxn=rxnlist[table[i][j]];
						dist2=0;
						for(d=0;d<dim;d++)
						{
							if((wpcode>>2*d&3)==0)
							{
								dist2+=(mptr1->pos[d]-mptr2->pos[d])*(mptr1->pos[d]-mptr2->pos[d]);
							}
							else if((wpcode>>2*d&3)==1)
							{
								pos2=sim->wlist[2*d+1]->pos-sim->wlist[2*d]->pos;
								dist2+=(mptr1->pos[d]-mptr2->pos[d]+pos2)*(mptr1->pos[d]-mptr2->pos[d]+pos2);
							}
							else
							{
								pos2=sim->wlist[2*d+1]->pos-sim->wlist[2*d]->pos;
								dist2+=(mptr1->pos[d]-mptr2->pos[d]-pos2)*(mptr1->pos[d]-mptr2->pos[d]-pos2);
							}
						}
						//Christine Have to check whether a molecule is on a surface or not and then apply correct bindrad (definitely have to change that for accounting for mol on different surfaces...)
						surf_num1=mptr1->pnl->srf->surface_number+1;
						surf_num2=mptr2->pnl->srf->surface_number+1;

						if(dist2<=rxn->bindrad2[surf_num1][surf_num2] && (rxn->prob[surf_num1][surf_num2]==1 || randCOD()<rxn->prob[surf_num1][surf_num2]) && mptr1->ident!=0 && mptr2->ident!=0)
						{
							morebireact_params.rxn_to_execute = rxn;
							morebireact_params.mol_ptr_1 = mptr1;
							morebireact_params.mol_ptr_2 = mptr2;
							morebireact_params.ll_ndx_1 = ll1;
							morebireact_params.mol_ndx_1 = m1;
							morebireact_params.ll_ndx_2 = ll2;
							morebireact_params.et = ETrxn2wrap;

							push_data_onto_stack( output_stack, &morebireact_params, sizeof(morebireact_params));
							*((int*) output_stack->stack_data) += 1;

						}
					}
				}
			}

			else // neighbor box, no wrapping
			for(m2=0;m2<nmol2;m2++)
			{
				mptr2=mlist2[m2];
				i=mptr1->ident*maxspecies+mptr2->ident;
				for(j=0;j<nrxn[i];j++)
				{
					rxn=rxnlist[table[i][j]];
					dist2=0;
					for(d=0;d<dim;d++)
					{
						dist2+=(mptr1->pos[d]-mptr2->pos[d])*(mptr1->pos[d]-mptr2->pos[d]);
					}
					//Christine Have to check whether a molecule is on a surface or not and then apply correct bindrad (definitely have to change that for accounting for mol on different surfaces...)
					surf_num1=mptr1->pnl->srf->surface_number+1;
					surf_num2=mptr2->pnl->srf->surface_number+1;
					if(dist2<=rxn->bindrad2[surf_num1][surf_num2] && (rxn->prob[surf_num1][surf_num2]==1 || randCOD()<rxn->prob[surf_num1][surf_num2]) && (mptr1->mstate!=MSsoln || mptr2->mstate!=MSsoln || !rxnXsurface(sim,mptr1,mptr2)) && mptr1->ident!=0 && mptr2->ident!=0)
					{
						morebireact_params.rxn_to_execute = rxn;
						morebireact_params.mol_ptr_1 = mptr1;
						morebireact_params.mol_ptr_2 = mptr2;
						morebireact_params.ll_ndx_1 = ll1;
						morebireact_params.mol_ndx_1 = m1;
						morebireact_params.ll_ndx_2 = ll2;
						morebireact_params.et = ETrxn2inter;

						push_data_onto_stack( output_stack, &morebireact_params, sizeof(morebireact_params));
						*((int*) output_stack->stack_data) += 1;
					}
				}
			}
		}
	}

	return NULL;
#endif
}

// We pass in the data from the stack, NOT the stack itself.
void*
check_for_intrabox_bireactions_threaded(void* data) {
#ifndef THREADING
	return NULL;
#else
	int new_num = 0;

	// Get the input params for this function and setup the ouput_stack properly.
	PARAMS_check_for_intrabox* intrabox_input_params = (PARAMS_check_for_intrabox*) data;

	simptr sim = intrabox_input_params->sim;
	int live_list_ndx_1 = intrabox_input_params->ll_ndx_1;
	int live_list_ndx_2 = intrabox_input_params->ll_ndx_2;
	int first_ndx = intrabox_input_params->first_ndx;
	int second_ndx = intrabox_input_params->second_ndx;;
	stack* output_stack = intrabox_input_params->output_stack;

	//VDEBUG_printf("In thread.  Reading params ( sim = %p, ll_ndx_1 = %d, first_ndx = %d, second_ndx = %d, ll_ndx_2 = %d, output_stack = %p)\n", intrabox_input_params->sim, intrabox_input_params->ll_ndx_1, intrabox_input_params->first_ndx, intrabox_input_params->second_ndx, intrabox_input_params->ll_ndx_2, intrabox_input_params->output_stack);

	push_data_onto_stack(output_stack, &new_num, sizeof(new_num));

	//VDEBUG_printf("Writing proposal data to %p\n", output_stack->stack_data + output_stack->current_size);
	//VDEBUG_printf("Initially num_elements_found = %d\n", *(int*) output_stack->stack_data);

	PARAMS_morebireact morebireact_params;

	//////////////////////////////////////////////////

	//	int dim,maxspecies,ll1,ll2,i,j,d,*nl,nmol2,b2,m1,m2,bmax,wpcode,nlist,maxlist;
	int surf_num1, surf_num2,dim,maxspecies,i,j,d,*nl,nmol2,m2,nlist,maxlist;
	int *nrxn,**table;
	//	double dist2,pos2;
	double dist2;
	rxnssptr rxnss;
	rxnptr rxn,*rxnlist;
	boxptr bptr;
	moleculeptr **live,*mlist2,mptr1,mptr2;

	rxnss=sim->rxnss[2];
	dim=sim->dim;
	live=sim->mols->live;
	maxspecies=rxnss->maxspecies;
	maxlist=rxnss->maxlist;
	nlist=sim->mols->nlist;
	nrxn=rxnss->nrxn;
	table=rxnss->table;
	rxnlist=rxnss->rxn;
	nl=sim->mols->nl;

	int mol_ndx;
	for(mol_ndx = first_ndx; mol_ndx != second_ndx; ++mol_ndx)
	{
		mptr1 = live[live_list_ndx_1][mol_ndx];
		bptr = mptr1->box;
		mlist2=bptr->mol[live_list_ndx_2];
		nmol2=bptr->nmol[live_list_ndx_2];
		for(m2=0;m2<nmol2 && mlist2[m2]!=mptr1;m2++)
		{
			mptr2=mlist2[m2];
			i=mptr1->ident*maxspecies+mptr2->ident;
			for(j=0;j<nrxn[i];j++)
			{
				rxn=rxnlist[table[i][j]];
				dist2=0;
				for(d=0;d<dim;d++)
				{
					dist2 += (mptr1->pos[d]-mptr2->pos[d])*(mptr1->pos[d]-mptr2->pos[d]);
				}

				//Christine Have to check whether a molecule is on a surface or not and then apply correct bindrad (definitely have to change that for accounting for mol on different surfaces...)
				surf_num1=mptr1->pnl->srf->surface_number+1;
				surf_num2=mptr2->pnl->srf->surface_number+1;
				if(dist2 <= rxn->bindrad2[surf_num1][surf_num2] && randCOD() < rxn->prob[surf_num1][surf_num2] &&
						(mptr1->mstate != MSsoln || mptr2->mstate!=MSsoln || !rxnXsurface(sim,mptr1,mptr2) ) &&
						mptr1->ident != 0 &&
						mptr2->ident != 0)
				{

					// Create a new set of input paramaters for more bireact and push them onto our output stack.
					morebireact_params.rxn_to_execute = rxn;
					morebireact_params.mol_ptr_1 = mptr1;
					morebireact_params.mol_ptr_2 = mptr2;
					morebireact_params.ll_ndx_1 = live_list_ndx_1;
					morebireact_params.mol_ndx_1 = mol_ndx;
					morebireact_params.ll_ndx_2 = live_list_ndx_2;

					// VDEBUG_printf("P morebireact: (%p, %p, %p, %d, %d, %d)\n", rxn, mptr1, mptr2, live_list_ndx_1, m1, live_list_ndx_2);

					push_data_onto_stack( output_stack, &morebireact_params, sizeof(morebireact_params));
					*(int*) output_stack->stack_data += 1;
					new_num += 1;
				}
			}
		}

	}

	return NULL;
#endif
}
//???????????????? end of new code
