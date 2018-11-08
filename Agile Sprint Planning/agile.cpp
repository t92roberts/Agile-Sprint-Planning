#include <ilcplex/ilocplex.h>
#include <vector>
#include <string>
#include <algorithm>
ILOSTLBEGIN

class Sprint {
public:
	IloInt sprintNumber, sprintCapacity, sprintValueBonus;

	Sprint() {};

	Sprint(IloInt num, IloInt cap, IloInt bonus) {
		this->sprintNumber = num;
		this->sprintCapacity = cap;
		this->sprintValueBonus = bonus;
	}

	string toString() {
		return ">> Sprint " + to_string(sprintNumber) +
			" (capacity: " + to_string(sprintCapacity) +
			", bonus: " + to_string(sprintValueBonus) + ")";
	}
};

class Story {
public:
	int storyNumber, storyPoints, businessValue;
	vector<int> dependencies;

	Story() {};

	Story(int num, int bv, int sp, vector<int> d) {
		this->storyNumber = num;
		this->businessValue = bv;
		this->storyPoints = sp;
		this->dependencies = d;
	}

	string toString() {
		return "\tStory " + to_string(storyNumber)
			+ " (business value: " + to_string(businessValue)
			+ ", story points: " + to_string(storyPoints)
			+ ", dependencies: " + printDependencies() + ")";
	}

	string printDependencies() {
		if (dependencies.size() == 0) {
			return "None";
		}
		else {
			string dependenciesString = "";

			for (int i = 0; i < dependencies.size(); ++i) {
				if (i == 0) {
					dependenciesString += to_string(dependencies[i]);
				}
				else {
					dependenciesString += ", " + to_string(dependencies[i]);
				}
			}

			return dependenciesString;
		}
	}
};

int randomIntInclusive(int min, int max) {
	return rand() % (max - min + 1) + min;
}

vector<Sprint> randomlyGenerateSprints(int numberOfSprints, int minCapacity, int maxCapacity) {
	vector<Sprint> sprintData;

	for (int i = 0; i < numberOfSprints; ++i) {
		int capacity = randomIntInclusive(minCapacity, maxCapacity);

		// Sprint(sprintNumber, sprintCapacity, sprintBonus)
		sprintData.push_back(Sprint(i, capacity, numberOfSprints - i));
	}

	return sprintData;
}

vector<Story> randomlyGenerateStories(int numberOfStories, int minBusinessValue, int maxBusinessValue, int minStoryPoints, int maxStoryPoints, int minDependencies, int maxDependencies) {
	vector<Story> storyData;

	for (int i = 0; i < numberOfStories; ++i) {
		int numberOfDependencies = randomIntInclusive(minDependencies, maxDependencies);
		vector<int> dependencies = {};

		for (int j = 0; j < numberOfDependencies; ++j) {
			int randomStory;
			bool isAlreadyDependency;

			do {
				// Stop a story being dependent on itself
				randomStory = randomIntInclusive(0, numberOfStories - 1);

				// Stop adding the same story as a dependency
				isAlreadyDependency = find(dependencies.begin(), dependencies.end(), randomStory) != dependencies.end();
			} while (randomStory == i || isAlreadyDependency);

			dependencies.push_back(randomStory);
		}
		int businessValue = randomIntInclusive(minBusinessValue, maxBusinessValue);
		int storyPoints = randomIntInclusive(minStoryPoints, maxStoryPoints);

		// Story(storyNumber, businessValue, storyPoints, {dependency1, ..., dependencyN})
		storyData.push_back(Story(i, businessValue, storyPoints, dependencies));
	}

	return storyData;
}

int main(int argc, char* argv[]) {
	// Seed the random number generator
	srand(time(NULL));

	// SPRINTS ///////////////////////////////////////////////////////////////////
	const int numberOfSprints = stoi(argv[1]);
	// randomlyGenerateSprints(numberOfSprints, minCapacity, maxCapacity)
	vector<Sprint> sprintData = randomlyGenerateSprints(numberOfSprints, 5, 13);

	/*vector<Sprint> sprintData = {
		Sprint(0, 7, 4),
		Sprint(1, 7, 3),
		Sprint(2, 7, 2),
		Sprint(3, 7, 1)
	}

	const int numberOfSprints = sprintData.size();*/

	// STORIES ///////////////////////////////////////////////////////////////////
	const int numberOfStories = numberOfSprints / 2;
	// randomlyGenerateStories(numberOfStories, minBusinessValue, maxBusinessValue, minStoryPoints, maxStoryPoints, minDependencies, maxDependencies)
	vector<Story> storyData = randomlyGenerateStories(numberOfStories, 1, 10, 1, 13, 0, 2);

	/*vector<Story> storyData = {
		Story(0, 2, 6, {}),
		Story(1, 8, 3, {0}),
		Story(2, 2, 1, {1}),
		Story(3, 6, 4, {2})
	};

	const int numberOfStories = storyData.size();*/

	IloEnv env;

	try {
		IloModel model(env);

		// 2D array of Boolean decision variables (sprints[i][j] == 1 means user story j is taken in sprint i)
		IloArray<IloBoolVarArray> sprints(env, numberOfSprints);

		// Total business value delivered (across the whole roadmap)
		IloNumExpr deliveredValue(env, 0);

		// Create Boolean decision variables and objective function
		for (int i = 0; i < numberOfSprints; ++i) {
			// Sum of story points taken in the sprint
			IloNumExpr storyPointsTaken(env, 0);

			sprints[i] = IloBoolVarArray(env, numberOfStories);

			for (int j = 0; j < numberOfStories; ++j) {
				// Create a Boolean variable
				sprints[i][j] = IloBoolVar(env);

				// Add business value (including sprint bonus), if story is taken in this sprint
				deliveredValue += storyData[j].businessValue * sprintData[i].sprintValueBonus * sprints[i][j];

				// Add story points, if story is taken in this sprint
				storyPointsTaken += storyData[j].storyPoints * sprints[i][j];
			}

			// Add constraint (per sprint): sum of story points taken <= the sprint's capacity
			model.add(storyPointsTaken <= sprintData[i].sprintCapacity);
		}

		// Story is only assiged to one (or no) sprint
		for (int j = 0; j < numberOfStories; ++j) {
			IloNumExpr numberOfTimesStoryIsUsed(env, 0);

			for (int i = 0; i < numberOfSprints; ++i) {
				numberOfTimesStoryIsUsed += sprints[i][j];
			}

			model.add(numberOfTimesStoryIsUsed <= 1);
		}

		//////////////////////////////////////////////////////////////////////
		for (int i = 0; i < numberOfSprints; ++i) {
			for (int j = 0; j < numberOfStories; ++j) {
				IloInt numberOfDependencies = storyData[j].dependencies.size();

				for (int d = 0; d < numberOfDependencies; ++d) {
					// The story number of the dependee
					IloInt storyToCheck = storyData[j].dependencies[d];

					// How many times the dependee story has been assigned before to this sprint
					IloNumExpr numberOfTimesDependeesPreAssigned(env, 0);

					for (int sprintLookback = 0; sprintLookback < i; ++sprintLookback) {
						numberOfTimesDependeesPreAssigned += sprints[sprintLookback][storyToCheck];
					}

					// sprints[i][j] == 1 means that dependent story j is taken in sprint i, so dependee story d must appear in previous sprints
					// sprints[i][j] == 0 means that dependent story j is not taken in sprint i, so dependee story d may or may not appear in previous sprints
					model.add(IloIfThen(env, sprints[i][j] == 1, numberOfTimesDependeesPreAssigned == 1));
				}
			}
		}
		//////////////////////////////////////////////////////////////////////

		// Objective function
		model.add(IloMaximize(env, deliveredValue));

		IloCplex cplex(model);

		if (cplex.solve()) {
			cout << endl << "Solution status: " << cplex.getStatus() << endl;
			cout << "Maximum (weighted) business value = " << cplex.getObjValue() << endl << endl;

			for (int i = 0; i < numberOfSprints; ++i) {
				// Print Sprint information
				cout << sprintData[i].toString() << endl;
				int businessValueDelivered = 0;

				for (int j = 0; j < numberOfStories; ++j) {
					// If the story was taken in this sprint, print its information
					if (cplex.getValue(sprints[i][j]) == 1) {
						businessValueDelivered += storyData[j].businessValue;
						cout << storyData[j].toString() << endl;
					}
				}

				cout << "\t[Delivered: " << to_string(businessValueDelivered) << " business value, "
					<< to_string(businessValueDelivered * sprintData[i].sprintValueBonus) << " weighted business value]" << endl;

				cout << endl;
			}
		}
		else {
			cout << " No solution found" << endl;
		}

	}
	catch (IloAlgorithm::CannotExtractException &e) {
		IloExtractableArray const &failed = e.getExtractables();
		std::cerr << "Failed to extract:" << std::endl;
		for (IloInt i = 0; i < failed.getSize(); ++i)
			std::cerr << failed[i] << std::endl;
	}
	catch (IloException& e) {
		cerr << "Concert exception caught: " << e.getMessage() << endl;
	}
	catch (...) {
		cerr << "Unknown exception caught" << endl;
	}

	env.end();

	return 0;
}