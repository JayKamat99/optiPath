#ifndef OMPL_GEOMETRIC_PLANNER_KOMO_
#define OMPL_GEOMETRIC_PLANNER_KOMO_

#include <ompl/geometric/planners/PlannerIncludes.h>
#include <limits>

namespace ompl
{
    namespace geometric
    {
        OMPL_CLASS_FORWARD(Planner_KOMO);

        class Planner_KOMO : public base::Planner
        {
        protected:
            void freeMemory();
			double bestCost = 10 /* std::numeric_limits<double>::infinity() */;
			std::string bestCostProperty()
            {
                return std::to_string(bestCost);
            }

        public:
            Planner_KOMO(const base::SpaceInformationPtr &si, const char* filename);
            virtual ~Planner_KOMO() override;

			base::PlannerStatus solve(const base::PlannerTerminationCondition &ptc) override;
			std::string filename;
        };  
    } // namespace  geometric
} //namespace ompl

#endif