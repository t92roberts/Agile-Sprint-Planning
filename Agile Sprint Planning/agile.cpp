#include <ilcplex/ilocplex.h>
#include <vector>
#include <string>
#include <algorithm>
ILOSTLBEGIN

class Sprint {
public:
	int sprintNumber, sprintCapacity, sprintValueBonus;

	Sprint() {};

	Sprint(int num, int cap, int bonus) {
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
	// TODO - vector<int> relatedStories;

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

// Returns a random int between min and max (both inclusive)
int randomInt(int min, int max) {
	return rand() % (max - min + 1) + min;
}

// Returns an int between min and max (inclusive)
// The probability of drawing min is highest, decreasingly linearly to max
int randomIntLinearlyWeighted(int min, int max) {
	// A list of values to be drawn from where (given max = n):
	// min appears n times
	// min + 1 appears n - 1 times
	vector<int> weightedDistribution;
	int numberOfRepetitions = max;

	for (int i = min; i <= max; ++i) {
		for (int j = 0; j <= numberOfRepetitions; ++j) {
			weightedDistribution.push_back(i);
		}

		--numberOfRepetitions;
	}

	// A random place in the new vector of weighted values
	int randomIndex = randomInt(0, weightedDistribution.size() - 1);

	return weightedDistribution[randomIndex];
}

// Returns an int between min and max (inclusive)
// The probability of drawing min is highest, decreasingly exponentially to max
int randomIntExponentiallyWeighted(int min, int max, int exponent) {
	vector<int> weightedDistribution;

	int numberOfRepetitions = 1;

	for (int i = max; i >= min; --i) {
		// Add terms
		for (int j = 0; j < numberOfRepetitions; ++j) {
			weightedDistribution.push_back(i);
		}

		numberOfRepetitions = numberOfRepetitions * exponent;
	}

	// A random place in the new vector of weighted values
	int randomIndex = randomInt(0, weightedDistribution.size() - 1);

	return weightedDistribution[randomIndex];
}

// Returns a vector of Sprint objects filled with random values
vector<Sprint> randomlyGenerateSprints(int numberOfSprints, int minCapacity, int maxCapacity) {
	vector<Sprint> sprintData;

	for (int i = 0; i < numberOfSprints; ++i) {
		int capacity = randomInt(minCapacity, maxCapacity);

		// Sprint(sprintNumber, sprintCapacity, sprintBonus)
		sprintData.push_back(Sprint(i, capacity, numberOfSprints - i));
	}

	return sprintData;
}

// Returns a vector of Story objects filled with random values
vector<Story> randomlyGenerateStories(int numberOfStories, int minBusinessValue, int maxBusinessValue, int minStoryPoints, int maxStoryPoints) {
	vector<Story> storyData;

	for (int i = 0; i < numberOfStories; ++i) {
		//int numberOfDependencies = randomIntLinearlyWeighted(0, numberOfStories - 1);
		int numberOfDependencies = randomIntExponentiallyWeighted(0, numberOfStories - 1, 2);

		vector<int> dependencies = {};

		// TO-DO (maybe) - STOP CIRCULAR DEPENDENCIES CAUSING DEADLOCKS
		///////////////////////////////////////////////////////////////
		for (int j = 0; j < numberOfDependencies; ++j) {
			int randomStory;
			bool isAlreadyDependency;

			do {
				randomStory = randomInt(0, numberOfStories - 1);

				isAlreadyDependency = find(dependencies.begin(), dependencies.end(), randomStory) != dependencies.end();
			} while (randomStory == i || isAlreadyDependency); // Stop a story being dependent on itself or adding duplicate dependencies

			dependencies.push_back(randomStory);
		}
		int businessValue = randomInt(minBusinessValue, maxBusinessValue);
		int storyPoints = randomInt(minStoryPoints, maxStoryPoints);

		storyData.push_back(Story(i, businessValue, storyPoints, dependencies));
	}

	return storyData;
}

int main(int argc, char* argv[]) {
	// Seed the random number generator
	srand(time(NULL));

	// The number of sprints available in the roadmap
	int numberOfSprints;
	// Holds the data about each sprint
	vector<Sprint> sprintData;

	// The number of stories in the product backlog
	int numberOfStories;
	// Holds the data about each user story
	vector<Story> storyData;

	// The maximum number of dependencies that a user story can have
	int maxStoryDependencies;
	
	// TO-DO Implement a random number generator that uses a discrete distribution
	// (so 0 dependencies is more likely than numberOfStories - 1 dependencies

	bool showSolution = true;

	switch (argc) {
	case 4:
		if (stoi(argv[3]) == 0) {
			showSolution = false;
		}

		// No break statement as case 4 is identical to case 3, with an extra step
	case 3:
		numberOfSprints = stoi(argv[1]);
		numberOfStories = stoi(argv[2]);

		storyData = randomlyGenerateStories(numberOfStories, 1, 10, 1, 13);
		sprintData = randomlyGenerateSprints(numberOfSprints, 5, 13);
		
		break;
	default:
		// Use some hard-coded test data
		sprintData = {
			Sprint(0, 7, 4),
			Sprint(1, 7, 3),
			Sprint(2, 7, 2),
			Sprint(3, 7, 1)
		};

		numberOfSprints = sprintData.size();

		storyData = {
			Story(0, 2, 6, {}),
			Story(1, 8, 3, {0}),
			Story(2, 2, 1, {1}),
			Story(3, 6, 4, {2})
		};

		numberOfStories = storyData.size();

		break;
	}

	IloEnv env;

	try {
		IloModel model(env);

		// 2D array of Boolean decision variables (sprints[i][j] == 1 means user story j is taken in sprint i)
		IloArray<IloBoolVarArray> sprints(env, numberOfSprints);

		// Total business value delivered (across the whole roadmap)
		IloNumExpr deliveredValue(env, 0);

		for (int i = 0; i < numberOfSprints; ++i) {
			// Sum of the story points taken in the sprint i
			IloNumExpr storyPointsTaken(env, 0);

			// Sprint i is represented by an array of Boolean decision variables
			sprints[i] = IloBoolVarArray(env, numberOfStories);

			for (int j = 0; j < numberOfStories; ++j) {
				// Create a Boolean decision variable
				sprints[i][j] = IloBoolVar(env);

				// Add business value (including sprint bonus), if story is taken in this sprint
				deliveredValue += storyData[j].businessValue * sprintData[i].sprintValueBonus * sprints[i][j];

				// Add story points, if story j is taken in sprint i
				storyPointsTaken += storyData[j].storyPoints * sprints[i][j];
			}

			// Add constraint (per sprint): sum of story points taken in sprint i <= sprint i's capacity
			model.add(storyPointsTaken <= sprintData[i].sprintCapacity);
		}

		// Story is only assiged to one (or no) sprint
		for (int j = 0; j < numberOfStories; ++j) {
			// How many times story j is assigned in sprint i
			IloNumExpr numberOfTimesStoryIsUsed(env, 0);

			for (int i = 0; i < numberOfSprints; ++i) {
				numberOfTimesStoryIsUsed += sprints[i][j];
			}

			// Story j can be included in sprint i <= 1 times
			model.add(numberOfTimesStoryIsUsed <= 1);
		}

		// Add dependency constraints
		for (int i = 0; i < numberOfSprints; ++i) {
			for (int j = 0; j < numberOfStories; ++j) {
				// How many dependencies the story has
				int numberOfDependencies = storyData[j].dependencies.size();

				for (int d = 0; d < numberOfDependencies; ++d) {
					// The story number of the dependee story
					int storyToCheck = storyData[j].dependencies[d];

					// How many times the dependee story has been assigned before to this sprint
					IloNumExpr numberOfTimesDependeesPreAssigned(env, 0);

					// Count how many times the dependee has been included between the first sprint and the current sprint
					for (int sprintLookback = 0; sprintLookback < i; ++sprintLookback) {
						numberOfTimesDependeesPreAssigned += sprints[sprintLookback][storyToCheck];
					}

					// sprints[i][j] == 1 means that dependent story j is taken in sprint i, so dependee story d must appear in previous sprints
					// sprints[i][j] == 0 means that dependent story j is not taken in sprint i, so dependee story d may or may not appear in previous sprints
					model.add(IloIfThen(env, sprints[i][j] == 1, numberOfTimesDependeesPreAssigned == 1));
				}
			}
		}

		// Objective function
		model.add(IloMaximize(env, deliveredValue));

		IloCplex cplex(model);

		if (cplex.solve()) {
			cout << endl << "Solution status: " << cplex.getStatus() << endl;
			cout << "Maximum (weighted) business value = " << cplex.getObjValue() << endl << endl;

			if (showSolution) {
				for (int i = 0; i < numberOfSprints; ++i) {
					// Print Sprint information
					cout << sprintData[i].toString() << endl;
					int businessValueDelivered = 0;

					for (int j = 0; j < numberOfStories; ++j) {
						// If the story was taken in this sprint, print the story's information
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
		}
		else {
			cout << " No solution found" << endl;
		}

	}
	catch (IloAlgorithm::CannotExtractException &e) {
		IloExtractableArray const &failed = e.getExtractables();
		std::cerr << "Failed to extract:" << std::endl;
		for (int i = 0; i < failed.getSize(); ++i)
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