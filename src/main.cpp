#include <iomanip>

#include <petscsys.h>
#include <yaml-cpp/yaml.h>

#include <petibm/parser.h>

#include "config.h"
#include "rollingpitching.h"

int main(int argc, char **argv)
{
    PetscErrorCode ierr;
    YAML::Node config;
    RollingPitchingSolver solver;

    ierr = PetscInitialize(&argc, &argv, nullptr, nullptr); CHKERRQ(ierr);
    ierr = PetscLogDefaultBegin(); CHKERRQ(ierr);

    // parse configuration files; store info in YAML node
    ierr = petibm::parser::getSettings(config); CHKERRQ(ierr);

    // initialize the decoupled IBPM solver
    ierr = solver.init(PETSC_COMM_WORLD, config); CHKERRQ(ierr);
    ierr = solver.ioInitialData(); CHKERRQ(ierr);
    ierr = PetscPrintf(PETSC_COMM_WORLD,
                       "Completed initialization stage\n"); CHKERRQ(ierr);

    PetscReal dt, t;
    PetscInt nstart, ite, nt, nsave;
    const YAML::Node &node = config["parameters"];
    dt = node["dt"].as<PetscReal>();
    nstart = node["startStep"].as<PetscInt>(0);
    nt = node["nt"].as<PetscInt>();
    nsave = node["nsave"].as<PetscInt>(1);
    t = nstart * dt;
    ite = nstart;

    while (ite < nstart + nt)
    {
        t += dt;
        ite += 1;
        ierr = solver.setCoordinatesBodies(t); CHKERRQ(ierr);
        ierr = solver.setVelocityBodies(t); CHKERRQ(ierr);
        if (ite % nsave == 0)
        {
            std::stringstream ss;
            std::string filepath;
            ss << std::setfill('0') << std::setw(7) << ite;
            filepath = config["output"].as<std::string>() + "/" + \
                       ss.str() + ".h5";
            ierr = solver.writeLagrangianHDF5(filepath); CHKERRQ(ierr);
            filepath = config["output"].as<std::string>() + "/" + \
                       ss.str() + ".3D";
            ierr = solver.writeBodyASCII(filepath); CHKERRQ(ierr);
        }
    }

    // destroy the decoupled IBPM solver
    ierr = solver.destroy(); CHKERRQ(ierr);

    ierr = PetscFinalize(); CHKERRQ(ierr);

    return 0;
}  // main
