#include<iostream>
#include<cmath>
#include<complex>
# define pi           3.14159265358979323846 
using namespace std;


complex<double>* FastFourierTransform(complex<double>*p,int n)
{

	if(n==1) return p; //Base Case To return 

	complex<double> om=complex<double>(cos(2*pi/n),sin(2*pi/n));  //Calculating value of omega

	complex<double> *pe= new complex<double>[n/2]; // Coefficents of even power

	complex<double> *po= new complex<double>[n/2]; //Coeeficents of odd power

	int k1=0,k2=0;
	for(int j=0;j<n;j++)
	{
		if(j%2==0){
			pe[k1++]=p[j]; //Assigning values of even ceofficents

		}
		else po[k2++]=p[j]; //Assigning value of odd coefficents


	}

	complex<double>*ye=FastFourierTransform(pe,n/2);//Recursive Call
	complex<double>*yo=FastFourierTransform(po,n/2);//Recursive Call

	complex<double>*y=new complex<double>[n];

	k1=0,k2=0;

	for(int i=0;i<n/2;i++)
	{
		y[i]=ye[k1]+pow(om,i)*yo[k2]; 
		y[i+n/2]=ye[k1]-pow(om,i)*yo[k2];

		k1++;
		k2++;

	}

	return y;
	
}



int main(int argc, char const *argv[])
{
	complex<double> r[4]={1,2,3,4};

	int n=sizeof(r)/sizeof(complex<double>);//Determining size of the array
	
	complex<double>*k=FastFourierTransform(r,n); //calling dastfouriertransform



	while(n--)
	{
		cout<<*k<<"  ";
		k++;
	}
	return 0;
}