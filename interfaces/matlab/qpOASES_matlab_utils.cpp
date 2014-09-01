/*
 *	This file is part of qpOASES.
 *
 *	qpOASES -- An Implementation of the Online Active Set Strategy.
 *	Copyright (C) 2007-2014 by Hans Joachim Ferreau, Andreas Potschka,
 *	Christian Kirches et al. All rights reserved.
 *
 *	qpOASES is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation; either
 *	version 2.1 of the License, or (at your option) any later version.
 *
 *	qpOASES is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *	See the GNU Lesser General Public License for more details.
 *
 *	You should have received a copy of the GNU Lesser General Public
 *	License along with qpOASES; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */


/**
 *	\file interfaces/matlab/qpOASES_matlab_utils.cpp
 *	\author Hans Joachim Ferreau, Alexander Buchner
 *	\version 3.0
 *	\date 2007-2014
 *
 *	Collects utility functions for Interface to Matlab(R) that
 *	enables to call qpOASES as a MEX function.
 *
 */



QPInstance::QPInstance(	int _nV, int _nC, BooleanType _isSimplyBounded
						)
{
	handle = s_nexthandle++;

	if ( _nC > 0 )
		isSimplyBounded = BT_FALSE;
	else
		isSimplyBounded = _isSimplyBounded;
	
	if ( isSimplyBounded == BT_TRUE )
	{
		sqp = 0;
		qpb = new QProblemB( _nV );
	}
	else
	{
		sqp = new SQProblem( _nV,_nC );
		qpb = 0;
	}

	H = 0;
	A = 0;
	Hir = 0; 
	Hjc = 0; 
	Air = 0; 
	Ajc = 0;
	Hv = 0;
	Av = 0;
}	


QPInstance::~QPInstance( )
{		
	deleteQPMatrices();

	if ( sqp != 0 )
	{
		delete sqp;
		sqp = 0;
	}

	if ( qpb != 0 )
	{
		delete qpb;
		qpb = 0;
	}
}


returnValue QPInstance::deleteQPMatrices( )
{
	if ( H != 0 )
	{
		delete H;
		H = 0;
	}

	if (Hv != 0)
	{
		delete[] Hv;
		Hv = 0;
	}
	
	if (Hjc != 0)
	{
		delete[] Hjc;
		Hjc = 0;
	}
	
	if (Hir != 0)
	{
		delete[] Hir;
		Hir = 0;
	}
	
	if ( A != 0 )
	{
		delete A;
		A = 0;
	}

	if (Av != 0)
	{
		delete[] Av;
		Av = 0;
	}
	
	if (Ajc != 0)
	{
		delete[] Ajc;
		Ajc = 0;
	}
	
	if (Air != 0)
	{
		delete[] Air;
		Air = 0;
	}

	return SUCCESSFUL_RETURN;
}


int QPInstance::getNV() const
{
    if ( sqp != 0 )
        return sqp->getNV();
    
    if ( qpb != 0 )
        return qpb->getNV();
    
    return 0;
}


int QPInstance::getNC() const
{
    if ( sqp != 0 )
        return sqp->getNC();
   
    return 0;
}


/*
 *	a l l o c a t e Q P r o b l e m I n s t a n c e
 */
int allocateQPInstance(	int nV, int nC, BooleanType isSimplyBounded, const Options *options
						)
{
	QPInstance* inst = new QPInstance( nV,nC,isSimplyBounded );

	if ( inst->sqp != 0 )
		inst->sqp->setOptions ( *options );
	
	if ( inst->qpb != 0 )
		inst->qpb->setOptions ( *options );

	g_instances.push_back (inst);
	return inst->handle;
}


/*
 *  g e t Q P r o b l e m I n s t a n c e
 */
QPInstance* getQPInstance( int handle )
{
	unsigned int ii;
	// TODO: this may become slow ...
	for (ii = 0; ii < g_instances.size (); ++ii)
		if (g_instances[ii]->handle == handle)
			return g_instances[ii];
	return 0;
}


/*
 *	d e l e t e Q P r o b l e m I n s t a n c e
 */
void deleteQPInstance( int handle )
{
	QPInstance *instance = getQPInstance (handle);
	if (instance != 0) {
		for (std::vector<QPInstance*>::iterator itor = g_instances.begin ();
		     itor != g_instances.end (); ++itor)
		     if ((*itor)->handle == handle) {
				g_instances.erase (itor);
				break;
			}
		delete instance;
	}
}



/*
 *	s m a r t D i m e n s i o n C h e c k
 */
returnValue smartDimensionCheck(	real_t** input, unsigned int m, unsigned int n, BooleanType emptyAllowed,
									const mxArray* prhs[], int idx
									)
{
	/* If index is negative, the input does not exist. */
	if ( idx < 0 )
	{
		*input = 0;
		return SUCCESSFUL_RETURN;
	}

	/* Otherwise the input has been passed by the user. */
	if ( mxIsEmpty( prhs[ idx ] ) )
	{
		/* input is empty */
		if ( ( emptyAllowed == BT_TRUE ) || ( idx == 0 ) ) /* idx==0 used for auxInput */
		{
			*input = 0;
			return SUCCESSFUL_RETURN;
		}
		else
		{
			char msg[MAX_STRING_LENGTH];
			if ( idx > 0 )
				snprintf(msg, MAX_STRING_LENGTH, "ERROR (qpOASES): Empty argument %d not allowed!", idx+1);
			myMexErrMsgTxt( msg );
			return RET_INVALID_ARGUMENTS;
		}
	}
	else
	{
		/* input is non-empty */
        if ( mxIsSparse( prhs[ idx ] ) == 0 )
        {
            if ( ( mxGetM( prhs[ idx ] ) == m ) && ( mxGetN( prhs[ idx ] ) == n ) )
            {
                *input = (real_t*) mxGetPr( prhs[ idx ] );
                return SUCCESSFUL_RETURN;
            }
            else
            {
                char msg[MAX_STRING_LENGTH];
				if ( idx > 0 )
					snprintf(msg, MAX_STRING_LENGTH, "ERROR (qpOASES): Input dimension mismatch for argument %d ([%ld,%ld] ~= [%d,%d]).",
							 idx+1, (long int)mxGetM(prhs[idx]), (long int)mxGetN(prhs[idx]), m, n);
				else /* idx==0 used for auxInput */
					snprintf(msg, MAX_STRING_LENGTH, "ERROR (qpOASES): Input dimension mismatch for some auxInput entry ([%ld,%ld] ~= [%d,%d]).",
							 (long int)mxGetM(prhs[idx]), (long int)mxGetN(prhs[idx]), m, n);
                myMexErrMsgTxt( msg );
                return RET_INVALID_ARGUMENTS;
            }
        }
        else
        {
            char msg[MAX_STRING_LENGTH];
			if ( idx > 0 )
				snprintf(msg, MAX_STRING_LENGTH, "ERROR (qpOASES): Vector argument %d must not be in sparse format!", idx+1);
			else /* idx==0 used for auxInput */
				snprintf(msg, MAX_STRING_LENGTH, "ERROR (qpOASES): auxInput entries must not be in sparse format!" );
			myMexErrMsgTxt( msg );
			return RET_INVALID_ARGUMENTS;
        }
	}

	return SUCCESSFUL_RETURN;
}



/*
 *	i s V a l i d D a t a
 */
BooleanType isValidData( const real_t* const data, int dim )
{
	int i;

	if ( data == 0 )
		return BT_TRUE;

	for ( i = 0; i < dim; ++i )
		if ( mxIsNaN(data[i]) == 1 )
			return BT_FALSE;

	return BT_TRUE;
}



/*
 *	c o n v e r t F o r t r a n T o C
 */
returnValue convertFortranToC( const real_t* const A_for, int nV, int nC, real_t* const A )
{
	int i,j;

	for ( i=0; i<nC; ++i )
		for ( j=0; j<nV; ++j )
			A[i*nV + j] = A_for[j*nC + i];

	return SUCCESSFUL_RETURN;
}


/*
 *	h a s O p t i o n s V a l u e
 */
BooleanType hasOptionsValue( const mxArray* optionsPtr, const char* const optionString, double** optionValue )
{
	mxArray* optionName = mxGetField( optionsPtr,0,optionString );

	if ( optionName == 0 )
	{
		char msg[MAX_STRING_LENGTH];
		snprintf(msg, MAX_STRING_LENGTH, "Option struct does not contain entry '%s', using default value instead!", optionString );
		mexWarnMsgTxt( msg );
		return BT_FALSE;
	}

	if ( ( mxIsEmpty(optionName) == false ) && ( mxGetM( optionName ) == 1 ) && ( mxGetN( optionName ) == 1 ) )
	{
		*optionValue = mxGetPr( optionName );
		return BT_TRUE;
	}
	else
	{
		char msg[MAX_STRING_LENGTH];
		snprintf(msg, MAX_STRING_LENGTH, "Option '%s' is not a scalar, using default value instead!", optionString );
		mexWarnMsgTxt( msg );
		return BT_FALSE;
	}
}


/*
 *	s e t u p O p t i o n s
 */
returnValue setupOptions( Options* options, const mxArray* optionsPtr, int& nWSRin, real_t& maxCpuTime )
{
	double* optionValue;
	int optionValueInt;

	/* Check for correct number of option entries;
	 * may occur, e.g., if user types options.<misspelledName> = <someValue>; */
	if ( mxGetNumberOfFields(optionsPtr) != 31 )
		mexWarnMsgTxt( "Options might be set incorrectly as struct has wrong number of entries!\n         Type 'help qpOASES_options' for further information." );


	if ( hasOptionsValue( optionsPtr,"maxIter",&optionValue ) == BT_TRUE )
		if ( *optionValue >= 0.0 )
			nWSRin = (int)*optionValue;

	if ( hasOptionsValue( optionsPtr,"maxCpuTime",&optionValue ) == BT_TRUE )
		if ( *optionValue >= 0.0 )
			maxCpuTime = *optionValue;

	if ( hasOptionsValue( optionsPtr,"printLevel",&optionValue ) == BT_TRUE )
	{
        #ifdef __SUPPRESSANYOUTPUT__
        options->printLevel = PL_NONE;
        #else
		optionValueInt = (int)*optionValue;
		options->printLevel = (REFER_NAMESPACE_QPOASES PrintLevel)optionValueInt;
        if ( options->printLevel < PL_DEBUG_ITER )
            options->printLevel = PL_DEBUG_ITER;
        if ( options->printLevel > PL_HIGH )
            options->printLevel = PL_HIGH;       
        #endif
	}

	if ( hasOptionsValue( optionsPtr,"enableRamping",&optionValue ) == BT_TRUE )
	{
		optionValueInt = (int)*optionValue;
		options->enableRamping = (REFER_NAMESPACE_QPOASES BooleanType)optionValueInt;
	}

	if ( hasOptionsValue( optionsPtr,"enableFarBounds",&optionValue ) == BT_TRUE )
	{
		optionValueInt = (int)*optionValue;
		options->enableFarBounds = (REFER_NAMESPACE_QPOASES BooleanType)optionValueInt;
	}

	if ( hasOptionsValue( optionsPtr,"enableFlippingBounds",&optionValue ) == BT_TRUE )
	{
		optionValueInt = (int)*optionValue;
		options->enableFlippingBounds = (REFER_NAMESPACE_QPOASES BooleanType)optionValueInt;
	}

	if ( hasOptionsValue( optionsPtr,"enableRegularisation",&optionValue ) == BT_TRUE )
	{
		optionValueInt = (int)*optionValue;
		options->enableRegularisation = (REFER_NAMESPACE_QPOASES BooleanType)optionValueInt;
	}

	if ( hasOptionsValue( optionsPtr,"enableFullLITests",&optionValue ) == BT_TRUE )
	{
		optionValueInt = (int)*optionValue;
		options->enableFullLITests = (REFER_NAMESPACE_QPOASES BooleanType)optionValueInt;
	}

	if ( hasOptionsValue( optionsPtr,"enableNZCTests",&optionValue ) == BT_TRUE )
	{
		optionValueInt = (int)*optionValue;
		options->enableNZCTests = (REFER_NAMESPACE_QPOASES BooleanType)optionValueInt;
	}

	if ( hasOptionsValue( optionsPtr,"enableDriftCorrection",&optionValue ) == BT_TRUE )
		options->enableDriftCorrection = (int)*optionValue;

	if ( hasOptionsValue( optionsPtr,"enableCholeskyRefactorisation",&optionValue ) == BT_TRUE )
		options->enableCholeskyRefactorisation = (int)*optionValue;

	if ( hasOptionsValue( optionsPtr,"enableEqualities",&optionValue ) == BT_TRUE )
	{
		optionValueInt = (int)*optionValue;
		options->enableEqualities = (REFER_NAMESPACE_QPOASES BooleanType)optionValueInt;
	}


	if ( hasOptionsValue( optionsPtr,"terminationTolerance",&optionValue ) == BT_TRUE )
		options->terminationTolerance = *optionValue;

	if ( hasOptionsValue( optionsPtr,"boundTolerance",&optionValue ) == BT_TRUE )
		options->boundTolerance = *optionValue;

	if ( hasOptionsValue( optionsPtr,"boundRelaxation",&optionValue ) == BT_TRUE )
		options->boundRelaxation = *optionValue;

	if ( hasOptionsValue( optionsPtr,"epsNum",&optionValue ) == BT_TRUE )
		options->epsNum = *optionValue;

	if ( hasOptionsValue( optionsPtr,"epsDen",&optionValue ) == BT_TRUE )
		options->epsDen = *optionValue;

	if ( hasOptionsValue( optionsPtr,"maxPrimalJump",&optionValue ) == BT_TRUE )
		options->maxPrimalJump = *optionValue;

	if ( hasOptionsValue( optionsPtr,"maxDualJump",&optionValue ) == BT_TRUE )
		options->maxDualJump = *optionValue;


	if ( hasOptionsValue( optionsPtr,"initialRamping",&optionValue ) == BT_TRUE )
		options->initialRamping = *optionValue;

	if ( hasOptionsValue( optionsPtr,"finalRamping",&optionValue ) == BT_TRUE )
		options->finalRamping = *optionValue;

	if ( hasOptionsValue( optionsPtr,"initialFarBounds",&optionValue ) == BT_TRUE )
		options->initialFarBounds = *optionValue;

	if ( hasOptionsValue( optionsPtr,"growFarBounds",&optionValue ) == BT_TRUE )
		options->growFarBounds = *optionValue;

	if ( hasOptionsValue( optionsPtr,"initialStatusBounds",&optionValue ) == BT_TRUE )
	{
		optionValueInt = (int)*optionValue;
		if ( optionValueInt < -1 ) 
			optionValueInt = -1;
		if ( optionValueInt > 1 ) 
			optionValueInt = 1;
		options->initialStatusBounds = (REFER_NAMESPACE_QPOASES SubjectToStatus)optionValueInt;
	}

	if ( hasOptionsValue( optionsPtr,"epsFlipping",&optionValue ) == BT_TRUE )
		options->epsFlipping = *optionValue;

	if ( hasOptionsValue( optionsPtr,"numRegularisationSteps",&optionValue ) == BT_TRUE )
		options->numRegularisationSteps = (int)*optionValue;

	if ( hasOptionsValue( optionsPtr,"epsRegularisation",&optionValue ) == BT_TRUE )
		options->epsRegularisation = *optionValue;

	if ( hasOptionsValue( optionsPtr,"numRefinementSteps",&optionValue ) == BT_TRUE )
		options->numRefinementSteps = (int)*optionValue;

	if ( hasOptionsValue( optionsPtr,"epsIterRef",&optionValue ) == BT_TRUE )
		options->epsIterRef = *optionValue;

	if ( hasOptionsValue( optionsPtr,"epsLITests",&optionValue ) == BT_TRUE )
		options->epsLITests = *optionValue;

	if ( hasOptionsValue( optionsPtr,"epsNZCTests",&optionValue ) == BT_TRUE )
		options->epsNZCTests = *optionValue;

	return SUCCESSFUL_RETURN;
}



/*
 *	s e t u p A u x i l i a r y I n p u t s
 */
returnValue setupAuxiliaryInputs(	const mxArray* auxInput, unsigned int nV, unsigned int nC,
									double** x0, double** guessedBoundsAndConstraints, double** guessedBounds, double** guessedConstraints
									)
{
	unsigned int i;
	mxArray* curField = 0;

	/* x0 */
	curField = mxGetField( auxInput,0,"x0" );
	if ( curField == NULL )
		mexWarnMsgTxt( "auxInput struct does not contain entry 'x0'!\n         Type 'help qpOASES_auxInput' for further information." );
	else
	{
		*x0 = mxGetPr(curField);
		if ( smartDimensionCheck( x0,nV,1, BT_TRUE,((const mxArray**)&curField),0 ) != SUCCESSFUL_RETURN )
			return RET_INVALID_ARGUMENTS;
	}

	/* guessedWorkingSet */
	curField = mxGetField( auxInput,0,"guessedWorkingSet" );
	if ( curField == NULL )
		mexWarnMsgTxt( "auxInput struct does not contain entry 'guessedWorkingSet'!\n         Type 'help qpOASES_auxInput' for further information." );
	else
	{
		*guessedBoundsAndConstraints = mxGetPr(curField);
		if ( smartDimensionCheck( guessedBoundsAndConstraints,nV+nC,1, BT_TRUE,((const mxArray**)&curField),0 ) != SUCCESSFUL_RETURN )
			return RET_INVALID_ARGUMENTS;

		if ( *guessedBoundsAndConstraints != 0 )
		{
			*guessedBounds = new double[nV];
			for (i = 0; i < nV; i++)
				(*guessedBounds)[i] = (*guessedBoundsAndConstraints)[i];

			if ( nC > 0 )
			{
				*guessedConstraints = new double[nC];
				for (i = 0; i < nC; i++)
					(*guessedConstraints)[i] = (*guessedBoundsAndConstraints)[i + nV];
			}
		}
	}

	return SUCCESSFUL_RETURN;
}


/*
 *	d e l e t e A u x i l i a r y I n p u t s
 */
returnValue deleteAuxiliaryInputs(	double** guessedBounds, double** guessedConstraints
									)
{
	if ( ( guessedBounds != 0 ) && ( *guessedBounds != 0 ) )
	{
		delete[] (*guessedBounds);
		*guessedBounds = 0;
	}

	if ( ( guessedConstraints != 0 ) && ( guessedConstraints != 0 ) )
	{
		delete[] (*guessedConstraints);
		*guessedConstraints = 0;
	}

	return SUCCESSFUL_RETURN;
}



/*
 *	a l l o c a t e O u t p u t s
 */
returnValue allocateOutputs(	int nlhs, mxArray* plhs[], int nV, int nC = 0, int nP = 1, int handle = -1
								)
{
	/* Create output vectors and assign pointers to them. */
	int curIdx = 0;

	/* handle */
	if ( handle >= 0 )
		plhs[curIdx++] = mxCreateDoubleMatrix( 1, 1, mxREAL );

	/* x */
	plhs[curIdx++] = mxCreateDoubleMatrix( nV, nP, mxREAL );

	if ( nlhs > curIdx )
	{
		/* fval */
		plhs[curIdx++] = mxCreateDoubleMatrix( 1, nP, mxREAL );

		if ( nlhs > curIdx )
		{
			/* exitflag */
			plhs[curIdx++] = mxCreateDoubleMatrix( 1, nP, mxREAL );

			if ( nlhs > curIdx )
			{
				/* iter */
				plhs[curIdx++] = mxCreateDoubleMatrix( 1, nP, mxREAL );

				if ( nlhs > curIdx )
				{
					/* lambda */
					plhs[curIdx++] = mxCreateDoubleMatrix( nV+nC, nP, mxREAL );

					if ( nlhs > curIdx )
					{
						/* setup auxiliary output struct */
						mxArray* auxOutput = mxCreateStructMatrix( 1,1,0,0 );
						int curFieldNum;
						
						/* working set */
						curFieldNum = mxAddField( auxOutput,"workingSet" );
						if ( curFieldNum >= 0 )
							mxSetFieldByNumber( auxOutput,0,curFieldNum,mxCreateDoubleMatrix( nV+nC, nP, mxREAL ) );

						curFieldNum = mxAddField( auxOutput,"cpuTime" );
						if ( curFieldNum >= 0 )
							mxSetFieldByNumber( auxOutput,0,curFieldNum,mxCreateDoubleMatrix( 1, nP, mxREAL ) );

						plhs[curIdx] = auxOutput;
					}
				}
			}
		}
	}
	
	return SUCCESSFUL_RETURN;
}


/*
 *	o b t a i n O u t p u t s
 */
returnValue obtainOutputs(	int k, QProblemB* qp, returnValue returnvalue, int _nWSRout, real_t _cpuTime,
							int nlhs, mxArray* plhs[], int nV, int nC = 0, int handle = -1
							)
{
	/* Create output vectors and assign pointers to them. */
	int curIdx = 0;

	/* handle */
	if ( handle >= 0 )
		plhs[curIdx++] = mxCreateDoubleScalar( handle );

	/* x */
	double* x = mxGetPr( plhs[curIdx++] );
	qp->getPrimalSolution( &(x[k*nV]) );

	if ( nlhs > curIdx )
	{
		/* fval */
		double* obj = mxGetPr( plhs[curIdx++] );
		obj[k] = qp->getObjVal( );

		if ( nlhs > curIdx )
		{
			/* exitflag */
			double* status = mxGetPr( plhs[curIdx++] );
			status[k] = (real_t)getSimpleStatus( returnvalue );

			if ( nlhs > curIdx )
			{
				/* iter */
				double* nWSRout = mxGetPr( plhs[curIdx++] );
				nWSRout[k] = (real_t) _nWSRout;

				if ( nlhs > curIdx )
				{
					/* lambda */
					double* y = mxGetPr( plhs[curIdx++] );
					qp->getDualSolution( &(y[k*(nV+nC)]) );

					if ( nlhs > curIdx )
					{
						
						mxArray* auxOutput = plhs[curIdx];
						mxArray* curField = 0;

						/* working set */
						curField = mxGetField( auxOutput,0,"workingSet" );
						double* workingSet = mxGetPr(curField);

						QProblem* problemPointer;
						problemPointer = dynamic_cast<QProblem*>(qp);

						// cast successful?
						if (problemPointer != NULL) {
							problemPointer->getWorkingSet( &(workingSet[k*(nV+nC)]) );
						} else {
							qp->getWorkingSet( &(workingSet[k*(nV+nC)]) );
						}

						/* cpu time */
						curField = mxGetField( auxOutput,0,"cpuTime" );
						double* cpuTime = mxGetPr(curField);
						cpuTime[0] = (real_t) _cpuTime;
					}
				}
			}
		}
	}
	
	return SUCCESSFUL_RETURN;
}



/*
 *	s e t u p H e s s i a n M a t r i x
 */
returnValue setupHessianMatrix(	const mxArray* prhsH, int nV,
								SymmetricMatrix** H, sparse_int_t** Hir, sparse_int_t** Hjc, real_t** Hv
								)
{
	if ( mxIsSparse( prhsH ) != 0 )
	{
		mwIndex *mat_ir = mxGetIr( prhsH );
		mwIndex *mat_jc = mxGetJc( prhsH );
		double *v = (double*)mxGetPr( prhsH );
		sparse_int_t nfill = 0;
		mwIndex i, j;
		BooleanType needInsertDiag;

		/* copy indices to avoid 64/32-bit integer confusion */
		/* also add explicit zeros on diagonal for regularization strategy */
		/* copy values, too */
		*Hir = new sparse_int_t[mat_jc[nV] + nV];
		*Hjc = new sparse_int_t[nV+1];
		*Hv = new real_t[mat_jc[nV] + nV];
        for (j = 0; j < nV; j++) 
		{
            needInsertDiag = BT_TRUE;
                
            (*Hjc)[j] = (sparse_int_t)(mat_jc[j]) + nfill;
            /* fill up to diagonal */
            for (i = mat_jc[j]; i < mat_jc[j+1]; i++) 
			{
                if ( mat_ir[i] == j )
                    needInsertDiag = BT_FALSE;
                    
                /* add zero diagonal element if not present */
                if ( ( mat_ir[i] > j ) && ( needInsertDiag == BT_TRUE ) )
                {
                    (*Hir)[i + nfill] = (sparse_int_t)j;
                    (*Hv)[i + nfill] = 0.0;
                    nfill++;
                    /* only add diag once */
                    needInsertDiag = BT_FALSE;
                }
                        
				(*Hir)[i + nfill] = (sparse_int_t)(mat_ir[i]);
				(*Hv)[i + nfill] = (real_t)(v[i]);
			}
		}
		(*Hjc)[nV] = (sparse_int_t)(mat_jc[nV]) + nfill;

		SymSparseMat *sH;
		*H = sH = new SymSparseMat(nV, nV, *Hir, *Hjc, *Hv);
		sH->createDiagInfo();
	}
	else
	{
		/* make a deep-copy in order to avoid modifying input data when regularising */
		real_t* H_for = (real_t*) mxGetPr( prhsH );
		real_t* H_mem = new real_t[nV*nV];
		memcpy( H_mem,H_for, nV*nV*sizeof(real_t) );

		*H = new SymDenseMat( nV,nV,nV, H_mem );
		(*H)->doFreeMemory( );
	}

	return SUCCESSFUL_RETURN;
}


/*
 *	s e t u p C o n s t r a i n t M a t r i x
 */
returnValue setupConstraintMatrix(	const mxArray* prhsA, int nV, int nC,
									Matrix** A, sparse_int_t** Air, sparse_int_t** Ajc, real_t** Av
									)
{
	if ( mxIsSparse( prhsA ) != 0 )
	{
		mwIndex i;
		long j;

		mwIndex *mat_ir = mxGetIr( prhsA );
		mwIndex *mat_jc = mxGetJc( prhsA );
		double *v = (double*)mxGetPr( prhsA );

		/* copy indices to avoid 64/32-bit integer confusion */
		*Air = new sparse_int_t[mat_jc[nV]];
		*Ajc = new sparse_int_t[nV+1];
		for (i = 0; i < mat_jc[nV]; i++)
			(*Air)[i] = (sparse_int_t)(mat_ir[i]);
		for (i = 0; i < nV + 1; i++)
			(*Ajc)[i] = (sparse_int_t)(mat_jc[i]);
		
		/* copy values, too */
		*Av = new real_t[(*Ajc)[nV]];
		for (j = 0; j < (*Ajc)[nV]; j++)
			(*Av)[j] = (real_t)(v[j]);

		*A = new SparseMatrix(nC, nV, *Air, *Ajc, *Av);
	}
	else
	{
		/* Convert constraint matrix A from FORTRAN to C style
		* (not necessary for H as it should be symmetric!). */
		real_t* A_for = (real_t*) mxGetPr( prhsA );
		real_t* A_mem = new real_t[nC*nV];
		convertFortranToC( A_for,nV,nC, A_mem );
		*A = new DenseMatrix(nC, nV, nV, A_mem );
		(*A)->doFreeMemory();
	}

	return SUCCESSFUL_RETURN;
}


/*
 *	end of file
 */
