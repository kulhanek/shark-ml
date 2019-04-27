#ifndef SHARK_TEST_DERIVATIVETESTHELPER_H
#define SHARK_TEST_DERIVATIVETESTHELPER_H

#include <vector>
#include <shark/LinAlg/Base.h>
#include <shark/Models/AbstractModel.h>

#include  <shark/Core/Random.h>
#include  <iostream>

namespace shark{
//estimates Derivative using the formula:
//df(x)/dx~=(f(x+e)-f(x-e))/2e
template<class Model,class Point>
std::vector<RealVector> estimateDerivative(Model& net,const Point& point,double epsilon=1.e-10){
	std::size_t outputSize = net(point).size();

	RealVector parameters=net.parameterVector();
	std::vector<RealVector> gradients(parameters.size(),RealVector(outputSize));
	for(size_t parameter=0;parameter!=parameters.size();++parameter){
		RealVector testPoint1=parameters;
		testPoint1(parameter)+=epsilon;
		net.setParameterVector(testPoint1);
		RealVector result1=net(point);

		RealVector testPoint2=parameters;
		testPoint2(parameter)-=epsilon;
		net.setParameterVector(testPoint2);
		RealVector result2=net(point);

		gradients[parameter]=(result1-result2)/(2*epsilon);
	}
	return gradients;
}
//input Derivative for Models
template<class Model,class Point>
std::vector<RealVector> estimateInputDerivative(Model& net,const Point& point,double epsilon=1.e-10){
	std::size_t outputSize = net(point).size();
	std::vector<RealVector> gradients(point.size(),RealVector(outputSize));
	for(size_t dim=0;dim!=point.size();++dim){
		RealVector testPoint1=point;
		testPoint1(dim)+=epsilon;
		RealVector result1=net(testPoint1);

		RealVector testPoint2=point;
		testPoint2(dim)-=epsilon;
		RealVector result2=net(testPoint2);
		gradients[dim]=(result1-result2)/(2*epsilon);
	}
	return gradients;
}

inline void testDerivative(const std::vector<RealVector>& g1,const std::vector<RealVector>& g2,double epsilon=1.e-5){
	BOOST_REQUIRE_EQUAL(g1.size(),g2.size());
	for(size_t output=0;output!=g1.size();++output){
		BOOST_REQUIRE_EQUAL(g1[output].size(),g2[output].size());
		BOOST_CHECK_SMALL(norm_2(g1[output]-g2[output]),epsilon);
	}
}
//general functions to estimate and test derivatives
template<class Model,class Point>
void testWeightedDerivative(Model& net,const Point& point,const RealVector& coefficients,double epsilon=1.e-5,double estimationEpsilon = 1.e-5){
	RealMatrix pointBatch(1,point.size());
	row(pointBatch,0)=point;
	boost::shared_ptr<State> state = net.createState();
	typename Model::BatchOutputType output; 
	net.eval(pointBatch,output,*state);

	std::vector<RealVector> derivative=estimateDerivative(net,point, estimationEpsilon);

	//check every coefficient independent of the others
	for(std::size_t coeff = 0; coeff!= coefficients.size(); ++coeff){
		RealMatrix coeffBatch(1,coefficients.size());
		coeffBatch.clear();
		coeffBatch(0,coeff)=coefficients(coeff);

		RealVector testGradient;
		net.weightedParameterDerivative(pointBatch, output, coeffBatch,*state,testGradient);
		//this makes the result again independent of the coefficient
		//provided that the computation is correct
		testGradient/=coefficients(coeff);

		//calculate error between both
		BOOST_REQUIRE_EQUAL(testGradient.size(),derivative.size());
		for(std::size_t i = 0; i != testGradient.size(); ++i){
			double error=sqr(testGradient(i)-derivative[i](coeff));
			BOOST_CHECK_SMALL(error,epsilon);
		}
	}
}

template<class Model,class Point>
void testWeightedInputDerivative(Model& net,const Point& point,const RealVector& coefficients,double epsilon=1.e-5, double estimationEpsilon=1.e-5){
	//now calculate the nets weighted gradient
	RealMatrix coeffBatch(1,coefficients.size());
	RealMatrix pointBatch(1,point.size());
	row(coeffBatch,0)=coefficients;
	row(pointBatch,0)=point;
	
	boost::shared_ptr<State> state = net.createState();
	typename Model::BatchOutputType output; 
	net.eval(pointBatch,output,*state);
	
	RealMatrix testGradient;
	net.weightedInputDerivative(pointBatch, output, coeffBatch,*state,testGradient);

	//calculate testresult
	//this is the weighted gradient calculated the naive way
	std::vector<RealVector> derivative=estimateInputDerivative(net,point,estimationEpsilon);
	RealVector resultGradient(derivative.size());
	for(size_t i=0;i!=derivative.size();++i)
		resultGradient(i)=inner_prod(derivative[i],coefficients);
	
	//calculate error between both
	double error=norm_inf(row(testGradient,0)-resultGradient);
	BOOST_CHECK_SMALL(error,epsilon);
	if(error > epsilon){
		std::cout<<"coefficients:"<<coefficients<<std::endl;
		std::cout<<"point:"<<point<<std::endl;
		std::cout<<"output:"<<row(testGradient,0)<<std::endl;
		std::cout<<"expected"<<resultGradient<<std::endl;
	}
}

///convenience function which does automatic sampling of points,parameters and coefficients
///and evaluates and tests the parameter derivative.
///it is assumed that the function has the methods inputSize() and  outputSize()
///samples are taken from the interval -10,10
template<class Model>
void testWeightedDerivative(Model& net,unsigned int numberOfTests = 1000, double epsilon=1.e-5,double estimationEpsilon = 1.e-5) {
	BOOST_CHECK_EQUAL(net.hasFirstParameterDerivative(),true);
	RealVector parameters(net.numberOfParameters());
	RealVector coefficients(net.outputShape().numElements());
	RealVector point(net.inputShape().numElements());
	for(unsigned int test = 0; test != numberOfTests; ++test){
		for(size_t i = 0; i != net.numberOfParameters();++i){
			parameters(i) = random::uni(random::globalRng,-1.0,1.0);
		}
		for(size_t i = 0; i != coefficients.size();++i){
			coefficients(i) = random::uni(random::globalRng,-1.0,1.0);
		}
		for(size_t i = 0; i != point.size();++i){
			point(i) = random::uni(random::globalRng,-1.0,1.0);
		}

		net.setParameterVector(parameters);
		testWeightedDerivative(net, point, coefficients, epsilon,estimationEpsilon);
	}
}
///convenience function which does automatic sampling of points,parameters and coefficients
///and evaluates and tests the input derivative.
///it is assumed that the function has the methods inputSize() and  outputSize()
///samples are taken from the interval -10,10
template<class Model>
void testWeightedInputDerivative(Model& net,unsigned int numberOfTests = 1000, double epsilon=1.e-5,double estimationEpsilon = 1.e-5) {
	BOOST_CHECK_EQUAL(net.hasFirstInputDerivative(),true);
	RealVector parameters(net.numberOfParameters());
	RealVector coefficients(net.outputShape().numElements());
	RealVector point(net.inputShape().numElements());
	for(unsigned int test = 0; test != numberOfTests; ++test){
		for(size_t i = 0; i != net.numberOfParameters();++i){
			parameters(i) = 1.0/net.numberOfParameters();//random::uni(random::globalRng,-1.0/net.numberOfParameters(),1.0/net.numberOfParameters());
		}
		for(size_t i = 0; i != coefficients.size();++i){
			coefficients(i) = random::uni(random::globalRng,-1.0,1.0);
		}
		for(size_t i = 0; i != point.size();++i){
			point(i) = random::uni(random::globalRng,-1.0,1.0);
		}

		net.setParameterVector(parameters);
		testWeightedInputDerivative(net, point, coefficients, epsilon,estimationEpsilon);
	}
}

///tests whether the derivatives computed separately are the same as the result returned by 
/// weightedInputDerivative and weightedParameterDerivative
template<class Model>
void testWeightedDerivativesSame(Model& net,unsigned int numberOfTests = 100, double epsilon = 1.e-10){
	BOOST_CHECK_EQUAL(net.hasFirstInputDerivative(),true);
	RealVector parameters(net.numberOfParameters());
	RealMatrix coeffBatch(10,net.outputShape().numElements());
	RealMatrix pointBatch(10,net.inputShape().numElements());
	for(unsigned int test = 0; test != numberOfTests; ++test){
		for(size_t i = 0; i != net.numberOfParameters();++i){
			parameters(i) = random::uni(random::globalRng,-1.0/net.numberOfParameters(),1.0/net.numberOfParameters());
		}
		for(std::size_t j = 0; j != 10; ++j){
			for(size_t i = 0; i != coeffBatch.size2();++i){
				coeffBatch(j,i) = random::uni(random::globalRng,-1.0/coeffBatch.size2(),1.0/coeffBatch.size2());
			}
			for(size_t i = 0; i != pointBatch.size2();++i){
				pointBatch(j,i) = random::uni(random::globalRng,-1.0/pointBatch.size2(),1.0/pointBatch.size2());
			}
		}
		net.setParameterVector(parameters);
		
		boost::shared_ptr<State> state = net.createState();
		typename Model::BatchOutputType output; 
		net.eval(pointBatch,output,*state);
		
		RealMatrix inputDerivative;
		RealVector parameterDerivative;
		net.weightedInputDerivative(pointBatch, output, coeffBatch,*state,inputDerivative);
		net.weightedParameterDerivative(pointBatch, output, coeffBatch,*state, parameterDerivative);
		RealMatrix testInputDerivative;
		RealVector testParameterDerivative;
		net.weightedDerivatives(pointBatch, output, coeffBatch,*state, testParameterDerivative, testInputDerivative);
		double errorInput = max(inputDerivative-testInputDerivative); 
		BOOST_CHECK_SMALL(errorInput,epsilon);
		BOOST_REQUIRE_EQUAL(parameterDerivative.size(), net.numberOfParameters());
		BOOST_REQUIRE_EQUAL(testParameterDerivative.size(), net.numberOfParameters());
		if(parameterDerivative.size() > 0){
			double errorParameter = max(parameterDerivative-testParameterDerivative); 
			BOOST_CHECK_SMALL(errorParameter,epsilon);
		}
	}
}

namespace detail{
//small helper functions which are used in testEval() to get the error between two samples
double elementEvalError(unsigned int a, unsigned int b){
	return (double) (a > b? a-b: b-a);
}
template<class T, class U>
double elementEvalError(T a, U b){
	return distance(a,b);
}
}

template<class T, class R>
void testBatchEval(AbstractModel<T, R>& model, typename Batch<T>::type const& sampleBatch){
	std::size_t size = batchSize(sampleBatch);

	//evaluate batch of inputs using a state and without stat.
	typename Batch<R>::type resultBatch = model(sampleBatch);
	typename Batch<R>::type resultBatch2;
	boost::shared_ptr<State> state = model.createState();
	model.eval(sampleBatch,resultBatch2,*state);
	
	//sanity check. if we don't get a result for every input something is seriously broken
	BOOST_REQUIRE_EQUAL(batchSize(resultBatch),size);
	BOOST_REQUIRE_EQUAL(batchSize(resultBatch2),size);

	//eval every element of the batch independently and compare the batch result with it
	for(std::size_t i = 0; i != size; ++i){
		R result = model(getBatchElement(sampleBatch,i));
		double error = detail::elementEvalError(result, getBatchElement(resultBatch,i));
		double error2 = detail::elementEvalError(result, getBatchElement(resultBatch2,i));
		BOOST_CHECK_SMALL(error, 1.e-7);
		BOOST_CHECK_SMALL(error2, 1.e-7);
	}
}

}
#endif
