/* ----------------------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   http://lammps.sandia.gov, Sandia National Laboratories
   Steve Plimpton, sjplimp@sandia.gov

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

#include <math.h>
#include <string.h>
#include "compute_angle_local.h"
#include "atom.h"
#include "atom_vec.h"
#include "molecule.h"
#include "update.h"
#include "domain.h"
#include "force.h"
#include "angle.h"
#include "math_const.h"
#include "memory.h"
#include "error.h"

using namespace LAMMPS_NS;
using namespace MathConst;

#define DELTA 10000

/* ---------------------------------------------------------------------- */

ComputeAngleLocal::ComputeAngleLocal(LAMMPS *lmp, int narg, char **arg) :
  Compute(lmp, narg, arg),
  vlocal(NULL), alocal(NULL)
{
  if (narg < 4) error->all(FLERR,"Illegal compute angle/local command");

  if (atom->avec->angles_allow == 0)
    error->all(FLERR,"Compute angle/local used when angles are not allowed");

  local_flag = 1;
  nvalues = narg - 3;
  if (nvalues == 1) size_local_cols = 0;
  else size_local_cols = nvalues;

  tflag = eflag = -1;
  nvalues = 0;

  for (int iarg = 3; iarg < narg; iarg++) {
    if (strcmp(arg[iarg],"theta") == 0) tflag = nvalues++;
    else if (strcmp(arg[iarg],"eng") == 0) eflag = nvalues++;
    else error->all(FLERR,"Invalid keyword in compute angle/local command");
  }

  nmax = 0;
  vlocal = NULL;
  alocal = NULL;
}

/* ---------------------------------------------------------------------- */

ComputeAngleLocal::~ComputeAngleLocal()
{
  memory->destroy(vlocal);
  memory->destroy(alocal);
}

/* ---------------------------------------------------------------------- */

void ComputeAngleLocal::init()
{
  if (force->angle == NULL)
    error->all(FLERR,"No angle style is defined for compute angle/local");

  // do initial memory allocation so that memory_usage() is correct

  ncount = compute_angles(0);
  if (ncount > nmax) reallocate(ncount);
  size_local_rows = ncount;
}

/* ---------------------------------------------------------------------- */

void ComputeAngleLocal::compute_local()
{
  invoked_local = update->ntimestep;

  // count local entries and compute angle info

  ncount = compute_angles(0);
  if (ncount > nmax) reallocate(ncount);
  size_local_rows = ncount;
  ncount = compute_angles(1);
}

/* ----------------------------------------------------------------------
   count angles and compute angle info on this proc
   only count if 2nd atom is the one storing the angle
   all atoms in interaction must be in group
   all atoms in interaction must be known to proc
   if angle is deleted (type = 0), do not count
   if angle is turned off (type < 0), still count
   if flag is set, compute requested info about angle
   if angle is turned off (type < 0), energy = 0.0
------------------------------------------------------------------------- */

int ComputeAngleLocal::compute_angles(int flag)
{
  int i,m,n,na,atom1,atom2,atom3,imol,iatom,atype;
  tagint tagprev;
  double delx1,dely1,delz1,delx2,dely2,delz2;
  double rsq1,rsq2,r1,r2,c;
  double *tbuf,*ebuf;

  double **x = atom->x;
  tagint *tag = atom->tag;
  int *num_angle = atom->num_angle;
  tagint **angle_atom1 = atom->angle_atom1;
  tagint **angle_atom2 = atom->angle_atom2;
  tagint **angle_atom3 = atom->angle_atom3;
  int **angle_type = atom->angle_type;
  int **mask = atom->mask;

  int *molindex = atom->molindex;
  int *molatom = atom->molatom;
  Molecule **onemols = atom->avec->onemols;

  int nlocal = atom->nlocal;
  int molecular = atom->molecular;

  if (flag) {
    if (nvalues == 1) {
      if (tflag >= 0) tbuf = vlocal;
      if (eflag >= 0) ebuf = vlocal;
    } else {
      if (tflag >= 0 && alocal) tbuf = &alocal[0][tflag];
      else tbuf = NULL;
      if (eflag >= 0 && alocal) ebuf = &alocal[0][eflag];
      else ebuf = NULL;
    }
  }

  Angle *angle = force->angle;

  m = n = 0;
  for (atom2 = 0; atom2 < nlocal; atom2++) {
    if (!(mask[atom2][groupbin] & groupbit)) continue;

    if (molecular == 1) na = num_angle[atom2];
    else {
      if (molindex[atom2] < 0) continue;
      imol = molindex[atom2];
      iatom = molatom[atom2];
      na = onemols[imol]->num_angle[iatom];
    }

    for (i = 0; i < na; i++) {
      if (molecular == 1) {
        if (tag[atom2] != angle_atom2[atom2][i]) continue;
        atype = angle_type[atom2][i];
        atom1 = atom->map(angle_atom1[atom2][i]);
        atom3 = atom->map(angle_atom3[atom2][i]);
      } else {
        if (tag[atom2] != onemols[imol]->angle_atom2[atom2][i]) continue;
        atype = onemols[imol]->angle_type[atom2][i];
        tagprev = tag[atom2] - iatom - 1;
        atom1 = atom->map(onemols[imol]->angle_atom1[atom2][i]+tagprev);
        atom3 = atom->map(onemols[imol]->angle_atom3[atom2][i]+tagprev);
      }

      if (atom1 < 0 || !(mask[atom1][groupbin] & groupbit)) continue;
      if (atom3 < 0 || !(mask[atom3][groupbin] & groupbit)) continue;
      if (atype == 0) continue;

      if (flag) {
        if (tflag >= 0) {
          delx1 = x[atom1][0] - x[atom2][0];
          dely1 = x[atom1][1] - x[atom2][1];
          delz1 = x[atom1][2] - x[atom2][2];
          domain->minimum_image(delx1,dely1,delz1);

          rsq1 = delx1*delx1 + dely1*dely1 + delz1*delz1;
          r1 = sqrt(rsq1);

          delx2 = x[atom3][0] - x[atom2][0];
          dely2 = x[atom3][1] - x[atom2][1];
          delz2 = x[atom3][2] - x[atom2][2];
          domain->minimum_image(delx2,dely2,delz2);

          rsq2 = delx2*delx2 + dely2*dely2 + delz2*delz2;
          r2 = sqrt(rsq2);

          // c = cosine of angle

          c = delx1*delx2 + dely1*dely2 + delz1*delz2;
          c /= r1*r2;
          if (c > 1.0) c = 1.0;
          if (c < -1.0) c = -1.0;
          tbuf[n] = 180.0*acos(c)/MY_PI;
        }

        if (eflag >= 0) {
          if (atype > 0)
            ebuf[n] = angle->single(atype,atom1,atom2,atom3);
          else ebuf[n] = 0.0;
        }
        n += nvalues;
      }

      m++;
    }
  }

  return m;
}

/* ---------------------------------------------------------------------- */

void ComputeAngleLocal::reallocate(int n)
{
  // grow vector_local or array_local

  while (nmax < n) nmax += DELTA;

  if (nvalues == 1) {
    memory->destroy(vlocal);
    memory->create(vlocal,nmax,"angle/local:vector_local");
    vector_local = vlocal;
  } else {
    memory->destroy(alocal);
    memory->create(alocal,nmax,nvalues,"angle/local:array_local");
    array_local = alocal;
  }
}

/* ----------------------------------------------------------------------
   memory usage of local data
------------------------------------------------------------------------- */

double ComputeAngleLocal::memory_usage()
{
  double bytes = nmax*nvalues * sizeof(double);
  return bytes;
}
