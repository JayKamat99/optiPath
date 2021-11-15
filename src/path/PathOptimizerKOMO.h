#ifndef OMPL_GEOMETRIC_PATH_OPTIMIZER_KOMO_
#define OMPL_GEOMETRIC_PATH_OPTIMIZER_KOMO_

#include <ompl/geometric/PathOptimizer.h>
#include <ompl/base/SpaceInformation.h>
#include <ompl/geometric/PathGeometric.h>
#include <string>

namespace ompl
{
    namespace geometric
    {
        OMPL_CLASS_FORWARD(PathOptimizerKOMO);

        class PathOptimizerKOMO : public PathOptimizer
        {
        private:
            /* data */

        public:
            // using PathOptimizer::PathOptimizer;
            PathOptimizerKOMO(base::SpaceInformationPtr si, const char* filename="");
            virtual ~PathOptimizerKOMO() = default;

            bool optimize(PathGeometric &path) override;
            std::string filename;
        };  
    } // namespace  geometric
} //namespace ompl

#endif
