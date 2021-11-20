#include <iostream>
#include <functional>
#include <ompl/base/SpaceInformation.h>
#include <ompl/base/ProblemDefinition.h>
#include <ompl/base/spaces/RealVectorStateSpace.h>
#include <ompl/geometric/planners/rrt/RRTstar.h>
#include <ompl/geometric/planners/rrt/RRTConnect.h>
#include <ompl/geometric/planners/est/EST.h>
#include <ompl/multilevel/planners/multimodal/LocalMinimaSpanners.h>
#include <ompl/geometric/SimpleSetup.h>
#include <ompl/tools/benchmark/Benchmark.h>

#include <ompl/base/samplers/ObstacleBasedValidStateSampler.h>
#include <ompl/base/samplers/GaussianValidStateSampler.h>

#include <path/PathOptimizerKOMO.h>
#include <path/Planner_KOMO.h>
#include <ompl/geometric/PathSimplifier.h>

#include <ompl/config.h>

#include <KOMO/komo.h>
#include <Kin/viewer.h>

#define PI 3.1412

namespace ob = ompl::base;
namespace og = ompl::geometric;
namespace om = ompl::multilevel;

unsigned int C_Dimension;

struct ValidityCheckWithKOMO {
	KOMO::Conv_KOMO_SparseNonfactored &nlp;
	ValidityCheckWithKOMO(KOMO::Conv_KOMO_SparseNonfactored &nlp) : nlp(nlp){}
	bool check(const ob::State *state)
	{
		const auto *State = state->as<ob::RealVectorStateSpace::StateType>();

		arr x_query;
		for (unsigned int i = 0; i < C_Dimension; i++){
			x_query.append((*State)[i]);
		}

		arr phi;
		nlp.evaluate(phi, NoArr, x_query);
		double tol = 1e-2;

		return std::abs(phi(0)) < tol;
	}
};

ob::ValidStateSamplerPtr allocOBValidStateSampler(const ob::SpaceInformation *si)
{
    return std::make_shared<ob::ObstacleBasedValidStateSampler>(si);
}

ob::ValidStateSamplerPtr allocGaussianValidStateSampler(const ob::SpaceInformation *si)
{
    return std::make_shared<ob::GaussianValidStateSampler>(si);
}

void VisualizePath(arrA configs, const char* filename = ""){
	static int Trajectory = 1;
	
	// Create an instance of KOMO
    rai::Configuration C;
    C.addFile(filename);
    KOMO komo;
    komo.verbose = 0;
    komo.setModel(C, true);
    
    komo.setTiming(1., configs.N, 5., 2);
	komo.add_qControlObjective({}, 1, 1.);

    komo.addObjective({1.}, FS_qItself, {}, OT_eq, {10}, configs(configs.N-1), 0);
    komo.addObjective({}, FS_accumulatedCollisions, {}, OT_eq, {1.});
    komo.add_collision(true);

    //use configs to initialize with waypoints
	komo.initWithWaypoints(configs, configs.N, false);
    komo.run_prepare(0);
	komo.plotTrajectory();
	std::string SaveToPath = std::string("z.vid/DisplayTrajectory_") + std::to_string(Trajectory) + "/";

	rai::ConfigurationViewer V;
	V.setPath(C, komo.x, "result", true);
	V.playVideo(true, 1., SaveToPath.c_str());
	Trajectory ++;
}

void benchmark(const char* filename = "../examples/Models/2D_arm.g", std::string planner_ = "PathOptimizerKOMO", bool benchmark = false)
{
	// set state validity checking based on KOMO
	rai::Configuration C;
	C.addFile(filename);
	KOMO komo;
	komo.setModel(C, true);
	komo.setTiming(1, 1, 1, 1);
	komo.addObjective({}, FS_accumulatedCollisions, {}, OT_eq, { 1 });
	komo.run_prepare(0);

	C_Dimension = C.getJointStateDimension();

	//Construct the state space we are planning in
	auto space(std::make_shared<ob::RealVectorStateSpace>(C_Dimension));

	ob::RealVectorBounds bounds(C_Dimension);
	bounds.setLow(-PI);
	bounds.setHigh(PI);
	space->setBounds(bounds);

	//create simple setup
	og::SimpleSetup ss(space);

    // set state validity checking for this space
	auto nlp = std::make_shared<KOMO::Conv_KOMO_SparseNonfactored>(komo, false);
	ValidityCheckWithKOMO checker(*nlp);

	ss.setStateValidityChecker([&checker](const ob::State *state) {
		return checker.check(state);
	});

	// si->setValidStateSamplerAllocator(allocOBValidStateSampler);
	// si->setValidStateSamplerAllocator(allocGaussianValidStateSampler);

    // create a start state
    ob::ScopedState<> start(space);
	for (unsigned int i=0; i<C.getJointStateDimension(); i++){
		start[i] = komo.getConfiguration_q(0).elem(i);
	}

	std::cout << start << std::endl;

    // create a goal state
    ob::ScopedState<> goal(space);
	for (unsigned int i=0; i<C.getJointStateDimension(); i++){
		if (i>3)	continue;
		goal[i] = komo.getConfiguration_q(0).elem(i)+1.5;
	}
	// goal = {2,0};

	std::cout << goal << std::endl;

    // create an instance of problem definition
    // auto pdef(std::make_shared<ob::ProblemDefinition>(si));

    // Set the start and goal states
    ss.setStartAndGoalStates(start, goal);
	// auto si = ss.getSpaceInformation();

	// //Define optimizer
	// // og::PathOptimizerPtr optimizer = std::make_shared<og::PathSimplifier>(si);
	// og::PathOptimizerPtr optimizer = std::make_shared<og::PathOptimizerKOMO>(si);


	if(benchmark)
	{
		// First we create a benchmark class:
		ompl::tools::Benchmark b(ss, "my experiment");

		auto si = ss.getSpaceInformation();
		std::vector<ob::SpaceInformationPtr> siVec;
		siVec.push_back(si);
		
		if (planner_ == "RRTstar"){
			auto planner1(std::make_shared<og::RRTstar>(si));
			b.addPlanner(planner1);
		}
		if (planner_ == "PathOptimizerKOMO"){
			//build KOMO object
			auto komo1(std::make_shared<KOMO>());
			auto planner(std::make_shared<om::LocalMinimaSpanners>(siVec));
			og::PathOptimizerPtr optimizer = std::make_shared<og::PathOptimizerKOMO>(si,filename);
			planner->setOptimizer(optimizer);
			b.addPlanner(planner);
		}
		if (planner_ == "PathSimplifier"){
			auto planner(std::make_shared<om::LocalMinimaSpanners>(siVec));
			og::PathOptimizerPtr optimizer = std::make_shared<og::PathSimplifier>(si);
			planner->setOptimizer(optimizer);
			b.addPlanner(planner);
		}
		if (planner_ == "KOMO"){
			auto planner(std::make_shared<og::Planner_KOMO>(si, filename));
			b.addPlanner(planner);
		}
		
		ompl::tools::Benchmark::Request req;
		req.maxTime = 5.0;
		req.maxMem = 100.0;
		req.runCount = 10;
		req.displayProgress = true;
		b.benchmark(req);
		
		// This will generate a .log file
		std::ostringstream oss;
		oss << "data/Benchmarks/benchmark_" << planner_ << ".log";
		b.saveResultsToFile(oss.str().c_str());
	}

	else{
		auto si = ss.getSpaceInformation();
		std::vector<ob::SpaceInformationPtr> siVec;
		siVec.push_back(si);
		auto planner1 = std::make_shared<om::LocalMinimaSpanners>(siVec);

		if (planner_ == "PathOptimizerKOMO"){
			og::PathOptimizerPtr optimizer = std::make_shared<og::PathOptimizerKOMO>(si,filename);
			planner1->setOptimizer(optimizer);
			ss.setPlanner(planner1);
		}
		else if (planner_ == "PathSimplifier"){
			og::PathOptimizerPtr optimizer = std::make_shared<og::PathOptimizerKOMO>(si,filename);
			planner1->setOptimizer(optimizer);
			ss.setPlanner(planner1);
		}
		else if(planner_ == "RRTstar"){
			auto planner(std::make_shared<og::RRTstar>(si));
			ss.setPlanner(planner);
		}
		else if(planner_ == "KOMO"){
			auto planner(std::make_shared<og::Planner_KOMO>(si, filename));
			ss.setPlanner(planner);
		}
		
		ss.setup();

		// attempt to solve the problem
		ob::PlannerStatus solved = ss.solve(20.0);

		if (solved == ob::PlannerStatus::StatusType::APPROXIMATE_SOLUTION)
			std::cout << "Found solution: APPROXIMATE_SOLUTION" << std::endl;
		else if (solved == ob::PlannerStatus::StatusType::EXACT_SOLUTION)
			std::cout << "Found solution: EXACT_SOLUTION" << std::endl;
		else if (solved == ob::PlannerStatus::StatusType::TIMEOUT)
			std::cout << "Found solution: TIMEOUT" << std::endl;
		else{
			std::cout << "No solution found: Invalid " << std::endl;
			return;
		}

		if(planner_ == "PathOptimizerKOMO" || "PathSimplifier"){ //This code is for visualization of the paths from PathOptimizer
			auto localMinimaTree = planner1->getLocalMinimaTree();
			int NumberOfMinima =  (int)localMinimaTree->getNumberOfMinima();
			int NumberOfLevels =  (int)localMinimaTree->getNumberOfLevel();

			for (int i=0; i<NumberOfLevels; i++){
				for (int j=0; j<NumberOfMinima; j++){
					std::cout << "\nNew path[" << i << j+1 << "] \n" << std::endl;
					auto path = std::dynamic_pointer_cast<ompl::geometric::PathGeometric>(localMinimaTree->getPath(i,j)->asPathPtr());
					//convert path to arrA
					arrA configs;
					for (auto state : (*path).getStates())
					{
						arr config;
						std::vector<double> reals;
						space->copyToReals(reals, state);
						for (double r : reals){
							config.append(r);
						}
						configs.append(config);
					}
					//Visualize in KOMO
					VisualizePath(configs, filename);
					std::dynamic_pointer_cast<ompl::geometric::PathGeometric>(localMinimaTree->getPath(i,j)->asPathPtr())->print(std::cout);
				}
			}
		}
		else{// This is for visualization of paths from other planners
			auto path = ss.getSolutionPath();
			arrA configs;
			for (auto state : path.getStates())
			{
				arr config;
				std::vector<double> reals;
				space->copyToReals(reals, state);
				for (double r : reals){
					config.append(r);
				}
				configs.append(config);
			}
			//Visualize in KOMO
			VisualizePath(configs, filename);
		}
	}
}

int main(int argc, char ** argv)
{
	std::cout << "OMPL version: " << OMPL_VERSION << std::endl;
	const char* filename = "";
	std::string planner_ = "";
	bool benchmark_ = false;
	if (argc<2){
		benchmark();
	}
	else{
		filename = argv[1];
		planner_ = argv[2];
		if (argv[3] == "true") benchmark_ = true;
		benchmark(filename, planner_, benchmark_);
	}
	return 0;
}
