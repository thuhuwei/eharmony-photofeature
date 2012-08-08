/*
 * eHfeatpyramid.cpp
 *
 * Hang Su
 * 2012-08 @ eH
 */
#include "eHfeatpyramid.h"
#include "eHmatrix.h"
#include <math.h>

static inline int min(int x, int y) { return (x <= y ? x : y); }
static inline int max(int x, int y) { return (x <= y ? y : x); }
static inline int round2int(double x) { return ((x-floor(x))>0.5 ? (int)ceil(x) : (int)floor(x));}

mat3d_ptr eHhog(const image_ptr img, int sbin);

facepyra_t* facepyra_create(const image_ptr im, int interval, int sbin, const int* maxsize) {
	facepyra_t* pyra = new facepyra_t;

	/* select padding, allowing for one cell in model to be visible
	 * Even padding allows for consistent spatial relations across 2X scales */
	int pady = max(maxsize[0]-1-1,0);
	int padx = max(maxsize[1]-1-1,0);
	double sc = pow(2.0, 1.0/(double)interval);

	int min_level = floor(log((double)min(im->sizy,im->sizx)/(5.0*sbin))/log(sc));
	image_t* scaled;
	image_t* tmp;
	pyra->len = min_level+interval+1;
	pyra->feat = new mat3d_ptr[pyra->len];
	pyra->scale = new double[pyra->len];
	for(int i=0;i<interval;i++) {
		/* first 2 octave */
		scaled = image_resize(im,(1.0/pow(sc,i)));
		pyra->feat[i]=eHhog(scaled, sbin/2);
		pyra->scale[i]=2.0/pow(sc,i);
		pyra->feat[i+interval] = eHhog(scaled,sbin);
		pyra->scale[i+interval] = 1.0/pow(sc,i);

		/* remaining octaves */
		for (int j=i+interval;; j+=interval){
			tmp = image_reduce(scaled);
			image_delete(scaled);
			scaled = tmp;
			pyra->feat[j+interval] = eHhog(scaled,sbin);
			pyra->scale[j+interval] = 0.5*pyra->scale[j];
			if(j+interval+interval>=pyra->len)
				break;
		}
	}

	size_t  pad[] = {pady+1, padx+1, 0};
	size_t  fill_start[3];
	size_t  fill_width[3];
	for (int i=0;i<pyra->len;i++) {
		/* add 1 to padding because feature generation deletes a 1-cell 
		 * wide border around the feature map */
		mat3d_pad(pyra->feat[i],pad,0);
		/* write boundary occlusion feature */
		fill_start[2]=pyra->feat[i]->sizz-1;fill_width[2]=1;
		fill_start[0]=0;fill_width[0]=pady+1;
		fill_start[1]=0;fill_width[1]=pyra->feat[i]->sizx;
		mat3d_fill(pyra->feat[i],fill_start,fill_width,1);
		fill_start[0]=pyra->feat[i]->sizy-pady-1;
		mat3d_fill(pyra->feat[i],fill_start,fill_width,1);
		fill_start[0]=pady+1;fill_width[0]=pyra->feat[i]->sizy-pady*2-2;
		fill_start[1]=0;fill_width[1]=padx+1;
		mat3d_fill(pyra->feat[i],fill_start,fill_width,1);
		fill_start[1]=pyra->feat[i]->sizx-padx-1;
		mat3d_fill(pyra->feat[i],fill_start,fill_width,1);
	}

	for (int i=0;i<pyra->len;i++) 
		pyra->scale[i] = (double)sbin/pyra->scale[i];
	pyra->interval = interval;
	pyra->imy = im->sizy;
	pyra->imx = im->sizx;
	
	return pyra;
}

void facepyra_delete(facepyra_t* pyra) {
	for (int i=0;i<pyra->len;i++) {
		mat3d_delete(pyra->feat[i]);
	}
	delete[] pyra->feat;
	delete[] pyra->scale;
	delete pyra;
}

