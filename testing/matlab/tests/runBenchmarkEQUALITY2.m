function [ successFlag ] = runBenchmarkEQUALITY2( nWSR )

    successFlag = 0;
	maxViolation = 0;

    clear H g A lb ub lbA ubA;
	
    try
        load 'benchmarkEQUALITY2.mat';
    catch
        successFlag = -1;
        return;
    end

   	if ( exist( 'A','var' ) )
        [nC,nV] = size(A);
    else
        nC = 0;
    end
    [nV,nP] = size(g);

    xOpt = zeros(nV,nP);
    yOpt = zeros(nV+nC,nP);
    objOpt = zeros(1,nP);
   
    %H = H;
    %lbA(65:320) = lbA(65:320) - 0.000000000;
    %ubA(65:320) = ubA(65:320) + 0.000000000;

	options = qpOASES_options( 'maxIter',nWSR, 'enableEqualities',1, 'printLevel',0 );
    %options = qpOASES_options( 'fast','maxIter',nWSR, 'printLevel',-2 );

	for i=1:nP
		%disp(i);

		if ( i == 1 )
			[QP,x,obj,status,nWSRout,lambda] = qpOASES_sequence( 'i',H,g(:,i),A,lb(:,i),ub(:,i),lbA(:,i),ubA(:,i),options );
		else
			[x,obj,status,nWSRout,lambda] = qpOASES_sequence( 'h',QP,g(:,i),lb(:,i),ub(:,i),lbA(:,i),ubA(:,i),options );
		end

		[ maxViolationTMP ] = getKktResidual( H,g(:,i),A,lb(:,i),ub(:,i),lbA(:,i),ubA(:,i), x,lambda );
		maxViolation = max( [maxViolation,maxViolationTMP] );

        xOpt(:,i) = x;
        yOpt(:,i) = lambda;
        objOpt(:,i) = obj;
	end
	
	qpOASES_sequence( 'c',QP );

    if ( ( maxViolation < 3e-7 ) && ( status == 0 ) )
        successFlag = 1;
    end
    
end
