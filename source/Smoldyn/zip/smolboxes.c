/* Steven Andrews, started 10/22/2001.
 This is a library of functions for the Smoldyn program.  See documentation
 called Smoldyn_doc1.pdf and Smoldyn_doc2.pdf.
 Copyright 2003-2011 by Steven Andrews.  This work is distributed under the terms
 of the Gnu General Public License (GPL). */

#include <float.h>
#include <stdio.h>
#include <string.h>
#include "Geometry.h"
#include "math2.h"
#include "random2.h"
#include "smoldyn.h"
#include "Zn.h"

#define CHECK(A) if(!(A)) goto failure
#define CHECKS(A,B) if(!(A)) {strncpy(erstr,B,STRCHAR-1);erstr[STRCHAR-1]='\0';goto failure;} else (void)0



/******************************************************************************/
/******************************** Virtual boxes *******************************/
/******************************************************************************/


/******************************************************************************/
/***************************** low level utilities ****************************/
/******************************************************************************/


/* box2pos.  Given a pointer to a box in bptr, this returns the coordinate of
the low and/or high corners of the box in poslo and poshi, respectively.  They
need to be pre-allocated to the system dimensionality.  If either point is
unwanted, enter NULL.  This requires that the min and size portions of the box
superstructure have been already set up. */
void box2pos(simptr sim,boxptr bptr,double *poslo,double *poshi) {
	int d,dim;
	double *size,*min;

	dim=sim->dim;
	size=sim->boxs->size;
	min=sim->boxs->min;
	if(poslo) for(d=0;d<dim;d++) poslo[d]=min[d]+bptr->indx[d]*size[d];
	if(poshi) for(d=0;d<dim;d++) poshi[d]=min[d]+(bptr->indx[d]+1)*size[d];
	return; }


/* pos2box returns a pointer to the box that includes the position given in pos,
which is a dim size vector.  If the position is outside the simulation volume, a
pointer to the nearest box is returned.  This routine assumes that the entire
box superstructure is set up. */
boxptr pos2box(simptr sim,double *pos) {
	int b,d,indx,dim;
	boxssptr boxs;

	dim=sim->dim;
	boxs=sim->boxs;
	b=0;
	for(d=0;d<dim;d++) {
		indx=(int)((pos[d]-boxs->min[d])/boxs->size[d]);
		if(indx<0) indx=0;
		else if(indx>=boxs->side[d]) indx=boxs->side[d]-1;
		b=boxs->side[d]*b+indx; }
	return boxs->blist[b]; }


/* boxrandpos.  Returns a uniformly distributed random point, in pos, that is
within the box bptr. */
void boxrandpos(simptr sim,double *pos,boxptr bptr) {
	int d;
	double *min,*size;

	min=sim->boxs->min;
	size=sim->boxs->size;
	for(d=0;d<sim->dim;d++)
		pos[d]=unirandCCD(min[d]+size[d]*bptr->indx[d],min[d]+size[d]*(bptr->indx[d]+1));
	return; }


/* Determines if any or all of the panel pnl is in the box bptr and returns 1 if
so and 0 if not.  For most panel shapes, this is sufficiently complicated that
this function just calls other functions in the library file Geometry.c. */
int panelinbox(simptr sim,panelptr pnl,boxptr bptr) {
	int dim,d,cross;
	double v1[DIMMAX],v2[DIMMAX],v3[DIMMAX],v4[DIMMAX],**point,*front;

	dim=sim->dim;
	box2pos(sim,bptr,v1,v2);							// v1 and v2 are set to corners of box
	for(d=0;d<dim;d++) {
		if(bptr->indx[d]==0) v1[d]=-DBL_MAX;
		if(bptr->indx[d]==sim->boxs->side[d]-1) v2[d]=DBL_MAX; }
	point=pnl->point;
	front=pnl->front;

	if(pnl->ps==PSrect) {
		if(dim==1) cross=Geo_PtInSlab(v1,v2,point[0],dim);
		else if(dim==2) {
			v3[0]=front[1]==0?1:0;
			v3[1]=front[1]==0?0:1;
			cross=Geo_LineXaabb2(point[0],point[1],v3,v1,v2); }
		else cross=Geo_RectXaabb3(point[0],point[1],point[3],point[0],v1,v2); }
	else if(pnl->ps==PStri) {
		if(dim==1) cross=Geo_PtInSlab(v1,v2,point[0],dim);
		else if(dim==2) cross=Geo_LineXaabb2(point[0],point[1],front,v1,v2);
		else cross=Geo_TriXaabb3(point[0],point[1],point[2],front,v1,v2); }
	else if(pnl->ps==PSsph) {
		if(dim==1) {
			if((point[0][0]-point[1][0]<v1[0] || point[0][0]-point[1][0]>=v2[0]) && (point[0][0]+point[1][0]<v1[0] || point[0][0]+point[1][0]>=v2[0])) cross=0;
			else cross=1; }
		else if(dim==2) cross=Geo_CircleXaabb2(point[0],point[1][0],v1,v2);
		else cross=Geo_SphsXaabb3(point[0],point[1][0],v1,v2); }
	else if(pnl->ps==PScyl) {
		if(dim==2) {
			v3[0]=point[0][0]+point[2][0]*front[0];
			v3[1]=point[0][1]+point[2][0]*front[1];
			v4[0]=point[1][0]+point[2][0]*front[0];
			v4[1]=point[1][1]+point[2][0]*front[1];
			cross=Geo_LineXaabb2(v3,v4,front,v1,v2);
			if(!cross) {
				v3[0]=point[0][0]-point[2][0]*front[0];
				v3[1]=point[0][1]-point[2][0]*front[1];
				v4[0]=point[1][0]-point[2][0]*front[0];
				v4[1]=point[1][1]-point[2][0]*front[1];
				cross=Geo_LineXaabb2(v3,v4,front,v1,v2); }}
		else {
			cross=Geo_CylsXaabb3(point[0],point[1],point[2][0],v1,v2); }}
	else if(pnl->ps==PShemi) {
		if(dim==2) cross=Geo_SemicXaabb2(point[0],point[1][0],point[2],v1,v2);
		else cross=Geo_HemisXaabb3(point[0],point[1][0],point[2],v1,v2); }
	else if(pnl->ps==PSdisk) {
		if(dim==2) {
			v3[0]=point[0][0]+point[1][0]*front[1];
			v3[1]=point[0][1]-point[1][0]*front[0];
			v4[0]=point[0][0]-point[1][0]*front[1];
			v4[1]=point[0][1]+point[1][0]*front[0];
			cross=Geo_LineXaabb2(v3,v4,front,v1,v2); }
		else cross=Geo_DiskXaabb3(point[0],point[1][0],front,v1,v2); }
	else cross=0;

	return cross; }


/* boxaddmol.  Adds molecule mptr, which belongs in live list ll, to the box
that is pointed to by mptr->box.  Returns 0 for success and 1 if memory could
not be allocated during box expansion. */
int boxaddmol(moleculeptr mptr,int ll) {
	boxptr bptr;

	bptr=mptr->box;
	if(bptr->nmol[ll]==bptr->maxmol[ll])
		if(expandbox(bptr,bptr->maxmol[ll]+1,ll)) return 1;
	bptr->mol[ll][bptr->nmol[ll]++]=mptr;
	return 0; }


/* boxremovemol.  Removes molecule mptr from the live list ll of the box that is
pointed to by mptr->box.  This function should only be called if mptr is known
to be listed in list ll of the box.  Before returning, mptr->box is set to NULL.
*/
void boxremovemol(moleculeptr mptr,int ll) {
	int m;
	boxptr bptr;

	bptr=mptr->box;
	for(m=bptr->nmol[ll]-1;bptr->mol[ll][m]!=mptr;m--);
	bptr->mol[ll][m]=bptr->mol[ll][--bptr->nmol[ll]];
	mptr->box=NULL;
	return; }


/******************************************************************************/
/****************************** memory management *****************************/
/******************************************************************************/

/* boxalloc.  boxalloc allocates and minimally initiallizes a new boxstruct.
Lists allocated are indx, which is size dim, and maxmol, nmol, and mol, each of
which are size nlist.  nlist may be entered as 0 to avoid allocating the latter
lists.  No molecule spaces are allocated. */
boxptr boxalloc(int dim,int nlist) {
	boxptr bptr;
	int d,ll;

	bptr=NULL;
	CHECK(dim>0);
	CHECK(bptr=(boxptr) malloc(sizeof(struct boxstruct)));
	bptr->indx=NULL;
	bptr->nneigh=0;
	bptr->midneigh=0;
	bptr->neigh=NULL;
	bptr->wpneigh=NULL;
	bptr->nwall=0;
	bptr->wlist=NULL;
	bptr->maxpanel=0;
	bptr->npanel=0;
	bptr->panel=NULL;
	bptr->maxmol=NULL;
	bptr->nmol=NULL;
	bptr->mol=NULL;

	CHECK(bptr->indx=(int*)calloc(dim,sizeof(int)));
	for(d=0;d<dim;d++) bptr->indx[d]=0;
	if(nlist) {
		CHECK(bptr->maxmol=(int*)calloc(nlist,sizeof(int)));
		for(ll=0;ll<nlist;ll++) bptr->maxmol[ll]=0;
		CHECK(bptr->nmol=(int*)calloc(nlist,sizeof(int)));
		for(ll=0;ll<nlist;ll++) bptr->nmol[ll]=0;
		CHECK(bptr->mol=(moleculeptr**)calloc(nlist,sizeof(moleculeptr*)));
		for(ll=0;ll<nlist;ll++) bptr->mol[ll]=NULL; }

	return bptr;

 failure:
	boxfree(bptr,nlist);
	return NULL; }


/* expandbox.  Expands molecule list ll within box bptr by n spaces.  If n is
negative, the box is shrunk and any molecule pointers that no longer fit are
simply left out.  This function may be used if the initial list size
(bptr->maxmol[ll]) was zero and can also be used to set the list size to zero.
The book keeping elements of the box are updated.  The function returns 0 if it
was successful and 1 if there was not enough memory for the request. */
int expandbox(boxptr bptr,int n,int ll) {
	moleculeptr *mlist;
	int m,maxmol,mn;

	maxmol=bptr->maxmol[ll]+n;
	if(maxmol>0) {
		mlist=(moleculeptr*) calloc(maxmol,sizeof(moleculeptr));
		if(!mlist) return 1;
		mn=(n>0)?bptr->maxmol[ll]:maxmol;
		for(m=0;m<mn;m++) mlist[m]=bptr->mol[ll][m]; }
	else {
		maxmol=0;
		mlist=NULL; }
	free(bptr->mol[ll]);
	bptr->mol[ll]=mlist;
	bptr->maxmol[ll]=maxmol;
	if(bptr->nmol[ll]>maxmol) bptr->nmol[ll]=maxmol;
	return 0;}


/* expandboxpanels.  Expands the list of panels in box bptr by n spaces.  If n �
0, this function ignores it, and does not shrink the box.  This updates the
maxpanel element.  Returns 0 for success and 1 for insufficient memory. */
int expandboxpanels(boxptr bptr,int n) {
	int maxpanel,p;
	panelptr *panel;

	if(n<=0) return 0;
	maxpanel=n+bptr->maxpanel;
	panel=(panelptr*) calloc(maxpanel,sizeof(panelptr));
	if(!panel) return 1;
	for(p=0;p<bptr->npanel;p++)
		panel[p]=bptr->panel[p];
	for(;p<maxpanel;p++)
		panel[p]=NULL;
	free(bptr->panel);
	bptr->panel=panel;
	bptr->maxpanel=maxpanel;
	return 0; }


/* boxfree.  Frees the box and all of its lists, although not the structures
pointed to by the lists.  nlist is the number of live lists. */
void boxfree(boxptr bptr,int nlist) {
	int ll;

	if(!bptr) return;
	if(bptr->mol) {
		for(ll=0;ll<nlist;ll++)
			free(bptr->mol[ll]); }
	free(bptr->mol);
	free(bptr->nmol);
	free(bptr->maxmol);
	free(bptr->panel);
	free(bptr->wlist);
	free(bptr->wpneigh);
	free(bptr->neigh);
	free(bptr->indx);
	free(bptr);
	return; }


/* boxesalloc.  boxesalloc allocates and initializes an array of nbox boxes,
including the boxes.  dim is the system dimensionality and nlist is the number
of live lists.  There is no additional initiallization beyond what is done in
boxalloc. */
boxptr *boxesalloc(int nbox,int dim,int nlist) {
	int b;
	boxptr *blist;

	blist=NULL;
	CHECK(nbox>0);
	CHECK(blist=(boxptr*)calloc(nbox,sizeof(boxptr)));
	for(b=0;b<nbox;b++) blist[b]=NULL;
	for(b=0;b<nbox;b++)
		CHECK(blist[b]=boxalloc(dim,nlist));
	return blist;

 failure:
	boxesfree(blist,nbox,nlist);
	return NULL; }


/* boxesfree.  Frees an array of boxes, including the boxes and the array.
nlist is the number of live lists. */
void boxesfree(boxptr *blist,int nbox,int nlist) {
	int b;

	if(!blist) return;
	for(b=0;b<nbox;b++) boxfree(blist[b],nlist);
	free(blist);
	return; }


/* boxssalloc.  Allocates and initializes a superstructure of boxes, including
arrays for the side, min, and size members, although the boxes are not added to
the structure, meaning that blist is set to NULL and nbox to 0. */
boxssptr boxssalloc(int dim) {
	boxssptr boxs;
	int d;

	boxs=NULL;
	CHECK(dim>0);
	CHECK(boxs=(boxssptr) malloc(sizeof(struct boxsuperstruct)));
	boxs->condition=SCinit;
	boxs->sim=NULL;
	boxs->nlist=0;
	boxs->mpbox=0;
	boxs->boxsize=0;
	boxs->boxvol=0;
	boxs->nbox=0;
	boxs->side=NULL;
	boxs->min=NULL;
	boxs->size=NULL;
	boxs->blist=NULL;

	CHECK(boxs->side=(int*)calloc(dim,sizeof(int)));
	for(d=0;d<dim;d++) boxs->side[d]=0;
	CHECK(boxs->min=(double*) calloc(dim,sizeof(double)));
	for(d=0;d<dim;d++) boxs->min[d]=0;
	CHECK(boxs->size=(double*) calloc(dim,sizeof(double)));
	for(d=0;d<dim;d++) boxs->size[d]=0;
	return boxs;

 failure:
	boxssfree(boxs);
	return NULL; }


/* boxssfree.  Frees a box superstructure, including the boxes.  nlist is the
number of live lists. */
void boxssfree(boxssptr boxs) {
	if(!boxs) return;
	boxesfree(boxs->blist,boxs->nbox,boxs->nlist);
	free(boxs->size);
	free(boxs->min);
	free(boxs->side);
	free(boxs);
	return; }


/******************************************************************************/
/*************************** data structure output ****************************/
/******************************************************************************/


/* boxoutput.  This displays the details of virtual boxes in the box
superstructure boxs that are numbered from blo to bhi-1.  To continue to the end
of the list, set bhi to -1.  This requires the system dimensionality in dim and
the number of live lists in nlist. */
void boxoutput(boxssptr boxs,int blo,int bhi,int dim) {
	int b,b2,b3,p,d,ll;
	boxptr bptr;

	printf("INDIVIDUAL BOX PARAMETERS\n");
	if(!boxs) {
		printf(" No box superstructure defined\n\n");
		return; }
	if(bhi<0 || bhi>boxs->nbox) bhi=boxs->nbox;
	for(b=blo;b<bhi;b++) {
		bptr=boxs->blist[b];
		printf(" Box %i: indx=(",b);
		for(d=0;d<dim-1;d++) printf("%i,",bptr->indx[d]);
		printf("%i), nwall=%i\n",bptr->indx[d],bptr->nwall);

		printf("  nneigh=%i midneigh=%i\n",bptr->nneigh,bptr->midneigh);
		if(bptr->neigh) {
			printf("   neighbors:");
			for(b2=0;b2<bptr->nneigh;b2++) {
				b3=indx2addZV(bptr->neigh[b2]->indx,boxs->side,dim);
				printf(" %i",b3); }
			printf("\n"); }
		if(bptr->wpneigh) {
			printf("  wrap code:");
			for(b2=0;b2<bptr->nneigh;b2++) printf(" %i",bptr->wpneigh[b2]);
			printf("\n"); }

		printf("  %i panels",bptr->npanel);
		if(bptr->npanel) {
			printf(": ");
			for(p=0;p<bptr->npanel;p++) {
				printf(" %s",bptr->panel[p]->pname); }}
		printf("\n");

		printf("  %i live lists:\n",boxs->nlist);
		printf("   max:");
		for(ll=0;ll<boxs->nlist;ll++) printf(" %i",bptr->maxmol[ll]);
		printf("\n   size:");
		for(ll=0;ll<boxs->nlist;ll++) printf(" %i",bptr->nmol[ll]);
		printf("\n"); }

	if(b<boxs->nbox) printf(" ...\n");
	printf("\n");
	return; }


/* boxssoutput.  Displays statistics about the box superstructure, including
total number of boxes, number on each side, dimensions, and the minimium
position.  It also prints out the requested and actual numbers of molecules
per box. */
void boxssoutput(simptr sim) {
	int dim,d,ll;
	boxssptr boxs;
	double flt1;

	printf("VIRTUAL BOX PARAMETERS\n");
	if(!sim || !sim->boxs) {
		printf(" No box superstructure defined\n\n");
		return; }
	dim=sim->dim;
	boxs=sim->boxs;
	printf(" %i boxes\n",boxs->nbox);
	printf(" Number of boxes on each side:");
	for(d=0;d<dim;d++) printf(" %i",boxs->side[d]);
	printf("\n");
	printf(" Minimum box position: ");
	for(d=0;d<dim;d++) printf(" %g",boxs->min[d]);
	printf("\n");
	if(boxs->boxsize) printf(" Requested box width: %g\n",boxs->boxsize);
	if(boxs->mpbox) printf(" Requested molecules per box: %g\n",boxs->mpbox);
	printf(" Box dimensions: ");
	for(d=0;d<dim;d++) printf(" %g",boxs->size[d]);
	printf("\n");
	printf(" Box volumes: %g\n",boxs->boxvol);
	flt1=0;
	for(ll=0;ll<sim->mols->nlist;ll++)
		if(sim->mols->listtype[ll]==MLTsystem) flt1+=sim->mols->nl[ll];
	flt1/=boxs->nbox;
	printf(" Molecules per box= %g\n",flt1);
	printf("\n");
	return; }


/* checkboxparams.  Checks and displays warning about various box parameters
such as molecules per box, box sizes, and number of panels in each box. */
int checkboxparams(simptr sim,int *warnptr) {
	int error,warn,b,dim,nmolec,ll;
	boxssptr boxs;
	double mpbox;
	char string[STRCHAR];
	boxptr bptr;

	error=warn=0;
	boxs=sim->boxs;
	dim=sim->dim;
	if(!boxs) {
		warn++;
		printf(" WARNING: no box structure defined\n");
		if(warnptr) *warnptr=warn;
		return 0; }

	if(boxs->condition!=SCok) {
		warn++;
		printf(" WARNING: box structure %s\n",simsc2string(boxs->condition,string)); }

	if(boxs->mpbox>100) {																// mpbox value
		warn++;
		printf(" WARNING: requested molecules per box, %g, is very high\n",boxs->mpbox); }
	else if(boxs->mpbox>0 && boxs->mpbox<1) {
		warn++;
		printf(" WARNING: requested molecules per box, %g, is very low\n",boxs->mpbox); }
	mpbox=boxs->mpbox>0?boxs->mpbox:10;

	for(b=0;b<boxs->nbox;b++) {
		bptr=boxs->blist[b];
		nmolec=0;																					// actual molecs per box
		for(ll=0;ll<sim->mols->nlist;ll++) nmolec+=bptr->nmol[ll];
		if(nmolec>10*mpbox) {
			warn++;
			printf(" WARNING: box (%s) has %i molecules in it, which is very high\n",Zn_vect2csvstring(bptr->indx,dim,string),nmolec); }

		if(bptr->npanel>20) {
			warn++;
			printf(" WARNING: box (%s) has %i panels in it, which is very high\n",Zn_vect2csvstring(bptr->indx,dim,string),bptr->npanel); }}

	if(warnptr) *warnptr=warn;
	return error; }



/******************************************************************************/
/********************************* structure set up ***************************/
/******************************************************************************/


/* boxsetcondition.  Sets the box superstructure condition to cond, if
appropriate.  Set upgrade to 1 if this is an upgrade, to 0 if this is a
downgrade, or to 2 to set the condition independent of its current value. */
void boxsetcondition(boxssptr boxs,enum StructCond cond,int upgrade) {
	if(!boxs) return;
	if(upgrade==0 && boxs->condition>cond) boxs->condition=cond;
	else if(upgrade==1 && boxs->condition<cond) boxs->condition=cond;
	else if(upgrade==2) boxs->condition=cond;
	if(boxs->condition<boxs->sim->condition) {
		cond=boxs->condition;
		simsetcondition(boxs->sim,cond==SCinit?SClists:cond,0); }
	return; }


/* boxsetsize.  Sets the requested box size.  info is a string that is
�molperbox� for the mpbox element, or is �boxsize� for the boxsize element, and
val is the requested value.  If the box superstructure has not been allocated
yet, this allocates it.  Returns 0 for success, 1 for failure to allocate memory,
2 for an illegal value, or 3 for the system dimensionality has not been set up
yet. */
int boxsetsize(simptr sim,char *info,double val) {
	boxssptr boxs;

	if(val<=0) return 2;
	if(!sim->boxs) {
		if(!sim->dim) return 3;
		boxs=boxssalloc(sim->dim);
		if(!boxs) return 1;
		boxs->sim=sim;
		sim->boxs=boxs;
		boxsetcondition(boxs,SCinit,0); }
	else
		boxs=sim->boxs;
	if(!strcmp(info,"molperbox")) boxs->mpbox=val;
	else if(!strcmp(info,"boxsize")) boxs->boxsize=val;
	boxsetcondition(boxs,SClists,0);
	return 0; }


/* setupboxes.  Sets up a superstructure of boxes, and puts things in the boxes,
including wall, panel, and molecule references.  It sets up the box
superstructure, then adds indicies to each box, then adds the box neighbor list
along with neighbor parameters, then adds wall references to each box, and
finally creates molecule lists for each box and sets both the box and molecule
references to point to each other.  The function returns 0 for successful
operation, 1 if it was unable to allocate sufficient memory, 2 if the simulation
dimensionality or walls weren�t set up yet, or 3 if the molecule superstructure
isn�t sufficiently set up (i.e. it�s condition is below SCparams).  At the end,
all simulation parameters having to do with boxes are set up.  This function can
run at set up or afterwards.  If the structure condition is SClists or lower,
this creates the boxes and sets their neighbors, walls, etc.; if the condition
is SCparams, this puts panels and molecules in boxes; if the condition is SCok,
this does nothing.  Overall, this function is very computationally intensive. */
int setupboxes(simptr sim) {
	int dim,d,m,mlo,mhi,nbox,b,b2,w,ll,ll1,mxml,er,nneigh,nwall,npanel;
	int *side,*indx,mpbox;
	boxssptr boxs;
	boxptr *blist,bptr;
	double flt1,flt2;
	int *ntemp,*wptemp;
	int nsrf,s,p;
	surfaceptr srf;
	moleculeptr mptr,*mlist;
	enum PanelShape ps;
	panelptr pnl;

	if(sim->dim<=0 || !sim->wlist) return 2;

	dim=sim->dim;
	boxs=sim->boxs;

	if(!boxs || boxs->condition<=SClists) {						// start of condition SClists
		if(!boxs) {																			// create superstructure if needed
			er=boxsetsize(sim,"molperbox",4);
			if(er) return 1;
			boxs=sim->boxs; }

		if(sim->mols && sim->mols->condition<SCparams) return 3;
		if(boxs->blist) {																// box superstructure
			boxesfree(boxs->blist,boxs->nbox,boxs->nlist);
			boxs->nbox=0; }
		side=boxs->side;
		mpbox=boxs->mpbox;
		if(mpbox<=0 && boxs->boxsize<=0) mpbox=5;
		if(mpbox>0) {
			flt1=systemvolume(sim);
			flt2=(double)molcount(sim,-1,MSall,NULL,-1);
			flt1=pow(flt2/mpbox/flt1,1.0/dim); }
		else flt1=1.0/boxs->boxsize;
		nbox=1;
		for(d=0;d<dim;d++) {
			boxs->min[d]=sim->wlist[2*d]->pos;
			side[d]=ceil((sim->wlist[2*d+1]->pos-sim->wlist[2*d]->pos)*flt1);
			if(!side[d]) side[d]=1;
			boxs->size[d]=(sim->wlist[2*d+1]->pos-sim->wlist[2*d]->pos)/side[d];
			nbox*=side[d]; }
		boxs->boxvol=1.0;
		for(d=0;d<dim;d++) boxs->boxvol*=boxs->size[d];
		boxs->nlist=sim->mols?sim->mols->nlist:0;
		boxs->nbox=nbox;
		blist=boxs->blist=boxesalloc(nbox,dim,boxs->nlist);
		if(!blist) return 1;

		for(b=0;b<nbox;b++) add2indxZV(b,blist[b]->indx,side,dim);	// box->indx

		if(sim->accur>=3) {
			ntemp=allocZV(intpower(3,dim)-1);								// neigh
			wptemp=allocZV(intpower(3,dim)-1);
			if(!ntemp || !wptemp) return 1;
			for(b=0;b<nbox;b++) {
				bptr=blist[b];
				indx=bptr->indx;
				if(sim->accur<6)
					nneigh=neighborZV(indx,ntemp,side,dim,0,NULL,&bptr->midneigh);
				else if(sim->accur<9) {
					for(w=0;w<2*dim;w++) wptemp[w]=sim->wlist[w]->type=='p';
					nneigh=neighborZV(indx,ntemp,side,dim,6,wptemp,&bptr->midneigh); }
				else {
					for(w=0;w<2*dim;w++) wptemp[w]=sim->wlist[w]->type=='p';
					nneigh=neighborZV(indx,ntemp,side,dim,7,wptemp,&bptr->midneigh); }
				if(nneigh==-1) return 1;
				w=0;
				if(sim->accur>=6) for(b2=0;b2<nneigh;b2++) w+=wptemp[b2];

				if(nneigh && nneigh!=bptr->nneigh) {
					free(bptr->neigh);
					free(bptr->wpneigh);
					bptr->wpneigh=NULL;
					bptr->neigh=(boxptr*) calloc(nneigh,sizeof(boxptr));
					if(!bptr->neigh) return 1;
					if(w) {
						bptr->wpneigh=allocZV(nneigh);
						if(!bptr->wpneigh) return 1; }
					bptr->nneigh=nneigh; }
				if(nneigh)
					for(b2=0;b2<nneigh;b2++) bptr->neigh[b2]=blist[ntemp[b2]];
				if(w) {
					for(b2=0;b2<nneigh;b2++) bptr->wpneigh[b2]=wptemp[b2]; }}

			neighborZV(NULL,NULL,NULL,0,-1,NULL,NULL);
			freeZV(ntemp);
			freeZV(wptemp); }

		for(b=0;b<nbox;b++) {														// box->nwall, wlist
			bptr=blist[b];
			indx=bptr->indx;
			nwall=0;
			for(d=0;d<dim;d++) {
				if(indx[d]==0) nwall++;
				if(indx[d]==side[d]-1) nwall++; }
			if(nwall && nwall!=bptr->nwall) {
				free(bptr->wlist);
				bptr->wlist=(wallptr*) calloc(nwall,sizeof(wallptr));
				if(!bptr->wlist) return 1;
				bptr->nwall=nwall; }
			if(nwall) {
				w=0;
				for(d=0;d<dim;d++) {
					if(indx[d]==0) bptr->wlist[w++]=sim->wlist[2*d];
					if(indx[d]==side[d]-1) bptr->wlist[w++]=sim->wlist[2*d+1]; }}}

		boxsetcondition(boxs,SCparams,1); }						// end of condition SClists

	if(boxs->condition==SCparams) {									// start of condition SCparams
		nbox=boxs->nbox;
		blist=boxs->blist;
		if(sim->srfss) {
			for(b=0;b<nbox;b++)
				blist[b]->npanel=0;
			nsrf=sim->srfss->nsrf;
			for(b=0;b<nbox;b++) {														// box->npanel, panel
				bptr=blist[b];
				npanel=0;
				for(s=0;s<nsrf;s++) {
					srf=sim->srfss->srflist[s];
					for(ps=0;ps<PSMAX;ps++)
						for(p=0;p<srf->npanel[ps];p++) {
							pnl=srf->panels[ps][p];
							if(panelinbox(sim,pnl,bptr)) npanel++; }}
				if(npanel && npanel>bptr->maxpanel) {
					er=expandboxpanels(bptr,npanel-bptr->maxpanel);
					if(er) return 1; }
				if(npanel) {
					for(s=0;s<nsrf;s++) {
						srf=sim->srfss->srflist[s];
						for(ps=0;ps<PSMAX;ps++)
							for(p=0;p<srf->npanel[ps];p++)
								if(panelinbox(sim,srf->panels[ps][p],bptr))
									bptr->panel[bptr->npanel++]=srf->panels[ps][p]; }}}}

		if(sim->mols) {												// mptr->box, box->maxmol, nmol, mol
			if(sim->mols->condition<SCparams) return 3;
			for(b=0;b<nbox;b++)
				for(ll=0;ll<boxs->nlist;ll++)
					blist[b]->nmol[ll]=0;
			for(ll1=-1;ll1<boxs->nlist;ll1++) {
				if(ll1==-1) {
					mlo=sim->mols->topd;
					mhi=sim->mols->nd;
					mlist=sim->mols->dead; }
				else {
					mlo=0;
					mhi=sim->mols->nl[ll1];
					mlist=sim->mols->live[ll1]; }
				for(m=mlo;m<mhi;m++) {
					mptr=mlist[m];
					bptr=pos2box(sim,mptr->pos);
					mptr->box=bptr;
					ll=sim->mols->listlookup[mptr->ident][mptr->mstate];
					bptr->nmol[ll]++; }}
			for(b=0;b<nbox;b++) {
				bptr=blist[b];
				for(ll=0;ll<boxs->nlist;ll++) {
					mxml=bptr->nmol[ll];
					bptr->nmol[ll]=0;
					if(bptr->maxmol[ll]<mxml) {
						er=expandbox(bptr,mxml*1.5-bptr->maxmol[ll],ll);
						if(er) return 1; }}}
			for(ll1=0;ll1<boxs->nlist;ll1++) {
				mlo=0;
				mhi=sim->mols->nl[ll1];
				mlist=sim->mols->live[ll1];
				for(m=mlo;m<mhi;m++) {
					mptr=mlist[m];
					ll=sim->mols->listlookup[mptr->ident][mptr->mstate];
					bptr=mptr->box;
					bptr->mol[ll][bptr->nmol[ll]++]=mptr; }}}

		boxsetcondition(boxs,SCok,1); }

	return 0; }




/******************************************************************************/
/*************************** core simulation functions ************************/
/******************************************************************************/


/* Given a line segment which is defined by the starting point pt1 and the
ending point pt2, and which is known to intersect the virtual box pointed to by
bptr, this returns a pointer to the next box along the line segment.  If the
current box is also the final one, NULL is returned.  Virtual boxes on the edge
of the system extend to infinity beyond the system walls, so this function
accurately tracks lines that are outside of the system volume. */
boxptr line2nextbox(simptr sim,double *pt1,double *pt2,boxptr bptr) {
	int dim,d,d2,boxside,boxside2,adrs,z1[DIMMAX],*side,sum,flag;
	double *size,*min,crsmin,edge,crs;

	if(pos2box(sim,pt2)==bptr) return NULL;
	dim=sim->dim;
	size=sim->boxs->size;
	side=sim->boxs->side;
	min=sim->boxs->min;
	crsmin=1.01;
	boxside2=0;
	d2=0;
	flag=0;
	for(d=0;d<dim;d++) {
		z1[d]=bptr->indx[d];
		if(pt2[d]!=pt1[d]) {
			boxside=(pt2[d]>pt1[d])?1:0;		// 1 for high side, 0 for low side
			sum=bptr->indx[d]+boxside;
			if(sum>0 && sum<side[d]) {
				edge=min[d]+(double)sum*size[d];		// absolute location of potential edge crossing
				crs=(edge-pt1[d])/(pt2[d]-pt1[d]);	// relative position of potential edge crossing on line
				if(crs<crsmin) {
					crsmin=crs;
					d2=d;
					boxside2=boxside;
					flag=0; }
				else if(crs==crsmin) {
					if(boxside==0 && boxside2==0 && (flag==0 || flag==2)) flag=2;
					else flag=1; }}}}

	if(flag) {
		for(d=0;d<dim;d++) {
			if(pt2[d]!=pt1[d]) {
				boxside=(pt2[d]>pt1[d])?1:0;
				sum=bptr->indx[d]+boxside;
				if(sum>0 && sum<side[d]) {
					edge=min[d]+(double)sum*size[d];
					crs=(edge-pt1[d])/(pt2[d]-pt1[d]);
					if(crs==crsmin && (boxside==1 || flag==2))
						z1[d]+=boxside?1:-1; }}}
		adrs=indx2addZV(z1,sim->boxs->side,dim);
		return sim->boxs->blist[adrs]; }

	if(crsmin==1.01) return NULL;
	z1[d2]+=boxside2?1:-1;
	adrs=indx2addZV(z1,sim->boxs->side,dim);
	return sim->boxs->blist[adrs]; }


/* reassignmolecs.  Reassigns molecules to boxes.  If diffusing is 1, only
molecules in lists that include diffusing molecules (sim->mols->diffuselist) are
reassigned; otherwise all lists are reassigned.  If reborn is 1, only molecules
that are reborn, meaning with indicies greater than or equal to topl[ll], are
reassigned; otherwise, entire lists are reassigned.  This assumes that all
molecules that are in the system are also in a box, meaning that the box element
of the molecule structure lists a box and that the mol list of that box lists
the molecule.  Molecules are arranged in boxes according to the location of the
pos element of the molecules.  Molecules outside the set of boxes are assigned
to the nearest box.  If more molecules belong in a box than actually fit, the
number of spaces is doubled using expandbox.  The function returns 0 unless
memory could not be allocated by expandbox, in which case it fails and returns
1. */
int reassignmolecs(simptr sim,int diffusing,int reborn) {
	int m,nmol,m2,ll;
	boxptr bptr1;
	moleculeptr mptr,*mlist,*mlist2;

	if(sim->boxs->nbox==1) return 0;
	for(ll=0;ll<sim->mols->nlist;ll++)
		if(sim->mols->listtype[ll]==MLTsystem)
			if(diffusing==0 || sim->mols->diffuselist[ll]==1) {
				nmol=sim->mols->nl[ll];
				mlist=sim->mols->live[ll];
				if(!reborn) m=0;
				else m=sim->mols->topl[ll];
				for(;m<nmol;m++) {
					mptr=mlist[m];
					bptr1=pos2box(sim,mptr->pos);
					if(mptr->box!=bptr1) {
						mlist2=mptr->box->mol[ll];		// remove from current box
						for(m2=0;mlist2[m2]!=mptr;m2++);
						mlist2[m2]=mlist2[--mptr->box->nmol[ll]];
						mptr->box=bptr1;								// add to new box
						// for parallelization, need: if(bptr1 is not within node) add molecule to send list.
						if(bptr1->nmol[ll]==bptr1->maxmol[ll])
							if(expandbox(bptr1,1+bptr1->nmol[ll],ll)) return 1;
						bptr1->mol[ll][bptr1->nmol[ll]++]=mptr; }}}
	return 0; }



