#ifndef __VirtualSolverCG_HPP__
#define __VirtualSolverCG_HPP__

#include <fstream>
#include "CG.hpp"
#include <cmath>
#include "HashMatrix.hpp"
#include <vector>
#include <complex>
#include "VirtualSolver.hpp"
#include "AFunction.hpp"

template<class I=int,class K=double>
struct HMatVirtPrecon: CGMatVirt<I,K> {
     typedef HashMatrix<I,K>  HMat;
    HMat *A;
    bool diag;
    //  Preco FF++
    Expression xx_del, code_del;
    const E_F0 * precon;
    Stack stack;
    KN<K> *xx;
    KN<int> *wcl;
    double tgv;
    HMatVirtPrecon(HMat *AA,const Data_Sparse_Solver * ds,Stack stk=0) :CGMatVirt<I,K>(AA->n),A(AA),diag(ds==0),
    xx_del(0),code_del(0),precon(0),stack(stk),wcl(0),xx(0)
    {  if(stack && ds && ds->precon)
    {
        const OneOperator * C = static_cast<const OneOperator *>(ds->precon);
        I n = AA->n;
        xx = new KN<K>(n);
        wcl = new KN<int>(n);
        WhereStackOfPtr2Free(stack)=new StackOfPtr2Free(stack);// FH mars 2005
        Type_Expr te_xx(CPValue(*xx));
        xx_del=te_xx.second;
        C_F0 e_xx(te_xx); // 1 undelete pointer
        code_del= C->code(basicAC_F0_wa(e_xx));
        precon =  to<KN_<K> >(C_F0(code_del,*C));// 2 undelete pointer
        throwassert(precon);
        K aii;
        tgv = ds->tgv;
        double tgve = ds->tgv;
        if(tgve <=0) tgve = 1e200;
        int ntgv =0;
        for (int i=0;i<n;i++)
            ntgv++,(*wcl)[i]-= std::abs(aii=(*A)(i,i))>tgve;
        
        if (verbosity>9) cout << " Precon  GC/GMRES : nb tgv in mat = "<< ntgv << endl;
    }
    else {stack=0;diag=true;} // no freefem++ precon
     }
    K * addmatmul(K *x,K *Ax) const
     {
        if(diag)
            for(int i=0; i<A->n; ++i)
                Ax[i] += std::abs((*A)(i,i)) ? x[i]/(*A)(i,i): x[i];
        else {// Call Precon ff++
            *xx=x;
            // cout << x[0] << "  ";
            *xx=GetAny<KN_<K> >((*precon)(stack));
            WhereStackOfPtr2Free(stack)->clean();
            //    cout << (xx)[0] << "  " << endl;
            K dii;
            for (int i=0;i<A->n;i++)
                Ax[i] += ((*wcl[i])? x[i]/tgv : (*xx)[i] );

        }
        return Ax;}
    ~HMatVirtPrecon()
    {
        if(xx) delete xx;
        if(wcl) delete wcl;
        if(stack) WhereStackOfPtr2Free(stack)->clean(); // FH mars 2005
        if(xx_del) delete  xx_del;
        if(code_del) delete  code_del;
    }
};
template<class I=int,class K=double>
class SolverCG: public VirtualSolver<I,K> {
public:
    
    typedef HashMatrix<I,K>  HMat;
    HMat *A;
    CGMatVirt<I,K> *pC;
    int verb,itermax,erronerr;
    double eps;
    SolverCG(HMat  &AA,double eeps=1e-6,int eoe=1,int v=1,int itx=0)
    :A(&AA),pC(0),verb(v),itermax(itx>0?itx:A->n/2),erronerr(eoe),eps(eeps)
    {
        cout << " SolverCG  " << A->n << " "<<  A->m <<" eps " << eps << " eoe " << eoe << " v " << verb << " " << itermax <<endl;
        pC = new HMatVirtPreconDiag(A);
        assert(A->n == A->m);
    }
 
    SolverCG(HMat  &AA,const Data_Sparse_Solver & ds,Stack stack)
    :A(&AA),pC(0),verb(ds.verb),itermax(ds.itmax>0 ?ds.itmax:A->n),erronerr(1),eps(ds.epsilon)
    {
        if(verb>1)
        std::cout << " SolverCG  " << A->n << "x"<<  A->m <<" eps " << eps << " eoe " << erronerr
                  << " v " << verb << " itmax " << itermax <<endl;
        assert(A->n == A->m);
        pC = new HMatVirtPrecon<I,K>(A,&ds,stack);
    }
        SolverCG() {delete pC;}
    void SetState(){}
    
    struct HMatVirt: CGMatVirt<I,K> {
        HMat *A;
        int t;
        HMatVirt(HMat *AA,int trans) :CGMatVirt<I,K>(AA->n),A(AA),t(trans) {}
        K * addmatmul(K *x,K *Ax) const { return A->addMatMul(x,Ax,t);}
    };
    struct HMatVirtPreconDiag: CGMatVirt<I,K> {
        HMat *A;
        HMatVirtPreconDiag(HMat *AA) :CGMatVirt<I,K>(AA->n),A(AA){}
        K * addmatmul(K *x,K *Ax) const { for(int i=0; i<A->n; ++i) Ax[i] += std::abs((*A)(i,i)) ? x[i]/(*A)(i,i): x[i]; return Ax;}
    };
    void dosolver(K *x,K*b,int N,int trans)
    {
        if(verb>2)
        std::cout <<"   SolverCG::dosolver" << N<< " "<< eps << " "<< itermax << " "<< verb << std::endl;
        HMatVirt AA(A,trans);
        //HMatVirtPreconDiag CC(A);
        int err=0;
        for(int k=0,oo=0; k<N; ++k, oo+= A->n )
        {
            int res=ConjugueGradient(AA,*pC,b+oo,x+oo,itermax,eps,verb);
            if ( res==0 ) err++;
        }
        if(err && erronerr) {  std::cerr << "Error: ConjugueGradient do not converge nb end ="<< err << std::endl; assert(0); }
    }

};


template<class I=int,class K=double>
class SolverGMRES: public VirtualSolver<I,K> {
public:
    
    typedef HashMatrix<I,K>  HMat;
    HMat *A;
    CGMatVirt<I,K> *pC;
    long verb,itermax,restart,erronerr;
    double eps;
    SolverGMRES(HMat  &AA,double eeps=1e-6,int eoe=1,int v=1,int rrestart=50,int itx=0)
    :A(&AA),pC(0), verb(v),itermax(itx>0?itx:A->n/2),restart(rrestart),
     erronerr(eoe),eps(eeps)
    {assert(A->n == A->m);
        pC = new HMatVirtPrecon<I,K>(A);
    }

    SolverGMRES(HMat  &AA,const Data_Sparse_Solver & ds,Stack stack)
    :A(&AA),pC(0),verb(ds.verb),itermax(ds.itmax>0 ?ds.itmax:A->n),restart(ds.NbSpace),erronerr(1),eps(ds.epsilon)
    {
        std::cout << " SolverGMRES  " << A->n << "x"<<  A->m <<" eps " << eps << " eoe " << erronerr
        << " v " << verb << "itsmx " << itermax <<endl;
        assert(A->n == A->m);
        pC = new HMatVirtPrecon<I,K>(A,&ds,stack);
    }

    ~SolverGMRES() {delete pC;}
    void SetState(){}
    
    struct HMatVirt: CGMatVirt<I,K> {
        HMat *A;
        int t;
        HMatVirt(HMat *AA,int trans=0) :CGMatVirt<I,K>(AA->n),A(AA),t(trans){}
        K * addmatmul(K *x,K *Ax) const { return  A->addMatMul(x,Ax,t);}
    };

    void dosolver(K *x,K*b,int N=0,int trans=1)
    {
        std::cout <<" SolverGMRES::dosolver" << N<< " "<< eps << " "<< itermax << " "<< verb << std::endl;
        HMatVirt AA(A,trans);
        //HMatVirtPreconDiag CC(A);
        int err=0;
        for(int k=0,oo=0; k<N; ++k, oo+= A->n )
        {
            bool res=fgmres(AA,*pC,b+oo,x+oo,eps,itermax,restart);
        
            if ( ! res ) err++;
        }
        if(err && erronerr) {  std::cerr << "Error: fgmres do not converge nb end ="<< err << std::endl; assert(0); }
    }
    
};

#endif
