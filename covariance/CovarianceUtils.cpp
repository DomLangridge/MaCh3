#include "covariance/CovarianceUtils.h"

namespace MaCh3Utils
{
  int GetNThreads()
  {
    #ifdef MULTITHREAD
    int nThreads = omp_get_max_threads();
    #else
    int nThreads = 1;
    #endif

    return nThreads;
  }



  //KS: Convert TMatrix to TH2D, mostly useful for making fancy plots
  TH2D* TMatrixIntoTH2D(const TMatrix &Matrix, std::string title)
  {
    TH2D* hMatrix = new TH2D(title.c_str(), title.c_str(), Matrix.GetNrows(), 0.0, Matrix.GetNrows(), Matrix.GetNcols(), 0.0, Matrix.GetNcols());
    for(int i = 0; i < Matrix.GetNrows(); i++)
    {
      for(int j = 0; j < Matrix.GetNcols(); j++)
      {
        //KS: +1 becasue there is offset in histogram relative to TMatrix
        hMatrix->SetBinContent(i+1,j+1, (Matrix)(i,j));
      }
    }

    return hMatrix;
  }

  //CW: Multi-threaded matrix multiplication
  TMatrixD MatrixMult(TMatrixD A, TMatrixD B) {
    double *C_mon = MatrixMult(A.GetMatrixArray(), B.GetMatrixArray(), A.GetNcols());
    TMatrixD C;
    C.Use(A.GetNcols(), A.GetNrows(), C_mon);
    return C;
  }

  double** MatrixMult(double **A, double **B, int n) {
    // First make into monolithic array
    double *A_mon = new double[n*n];
    double *B_mon = new double[n*n];

    #ifdef MULTITHREAD
    #pragma omp parallel for
    #endif
    for (int i = 0; i < n; ++i) {
      for (int j = 0; j < n; ++j) {
        A_mon[i*n+j] = A[i][j];
        B_mon[i*n+j] = B[i][j];
      }
    }
    //CW: Now call the monolithic calculator
    double *C_mon = MatrixMult(A_mon, B_mon, n);
    delete A_mon;
    delete B_mon;

    // Return the double pointer
    double **C = new double*[n];
    #ifdef MULTITHREAD
    #pragma omp parallel for
    #endif
    for (int i = 0; i < n; ++i) {
      C[i] = new double[n];
      for (int j = 0; j < n; ++j) {
        C[i][j] = C_mon[i*n+j];
      }
    }
    delete C_mon;

    return C;
  }

  double* MatrixMult(double *A, double *B, int n) {
    //CW: First transpose to increse cache hits
    double *BT = new double[n*n];
    #ifdef MULTITHREAD
    #pragma omp parallel for
    #endif
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
        BT[j*n+i] = B[i*n+j];
      }
    }

    // Now multiply
    double *C = new double[n*n];
    #ifdef MULTITHREAD
    #pragma omp parallel for
    #endif
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
        double sum = 0;
        for (int k = 0; k < n; k++) {
          sum += A[i*n+k]*BT[j*n+k];
        }
        C[i*n+j] = sum;
      }
    }
    delete BT;

    return C;
  }
}
