#include <ilcplex/ilocplex.h>
#include <vector>
#include <string>
#include <algorithm>
#include <set>
#include <tuple>
ILOSTLBEGIN

using namespace std;

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

	Story(int num, int bv, int sp) {
		this->storyNumber = num;
		this->businessValue = bv;
		this->storyPoints = sp;
	}

	Story(int num, int bv, int sp, vector<int> d) {
		this->storyNumber = num;
		this->businessValue = bv;
		this->storyPoints = sp;
		this->dependencies = d;
	}

	string toString() {
		return "Story " + to_string(storyNumber)
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
					dependenciesString += "Story " + to_string(dependencies[i]);
				}
				else {
					dependenciesString += ", Story " + to_string(dependencies[i]);
				}
			}

			return dependenciesString;
		}
	}
};

class Point {
public:
	int x, y;

	Point() {};

	Point(int newX, int newY) {
		this->x = newX;
		this->y = newY;
	}

	// Returns true if the first point is before the second point
	bool operator<(Point point) {
		if (this->x < point.x)
			return true;
		else if (this->x > point.x)
			return false;
		else if (this->y < point.y)
			return true;
		else
			return false;
	}
};

// Returns a random int between min and max (both inclusive) using a uniform distribution
int randomInt(int min, int max) {
	return rand() % (max - min + 1) + min;
}

// Returns a random position in the input vector according to the given probability distribution of getting each position
int randomIntDiscreteDistribution(vector<double> probabilities) {
	// Randomly generated percentage
	double randomPercentage = (double)(rand() % 1024) / 1024;

	// Threshold representing the upper limit of each probability band
	double threshold = probabilities[0];

	for (int i = 0; i < probabilities.size(); ++i) {
		// If the random percentage is within this vector position's probability threshold, return the position
		if (randomPercentage < threshold)
			return i;
		else
			// If not, extend the threshold to the next probability band
			threshold += probabilities[i + 1];
	}
}

// Uses the parametric equation of a geometric sequence to return a vector of doubles
vector<double> geometricSequence(double a, double r, double n) {
	vector<double> sequence;

	for (int i = 0; i < n; ++i) {
		sequence.push_back(a * pow(r, i));
	}

	return sequence;
}

/*
// Adapted from:
// GeeksforGeeks. (2018). Detect Cycle in a Directed Graph - GeeksforGeeks. [online] Available at: https://www.geeksforgeeks.org/detect-cycle-in-a-graph/ [Accessed 12 Nov. 2018].
// This function is a variation of DFSUytil() in https://www.geeksforgeeks.org/archives/18212 
*/
// Helper function that recursively steps through the directed graph to find a cycle
bool _isCyclic(int v, bool visited[], bool *recStack, vector <Story> allStories) {
	if (!visited[v]) {
		// Mark the current story as visited and part of recursion stack
		visited[v] = true;
		recStack[v] = true;

		// Recur for all the dependencies of this story 
		vector<int> dependencies = allStories[v].dependencies;

		vector<int>::iterator i;
		for (i = dependencies.begin(); i != dependencies.end(); ++i) {
			int dependeeStoryNumber = *i;
			if(!visited[dependeeStoryNumber] && _isCyclic(dependeeStoryNumber, visited, recStack, allStories))
				return true;
			else if (recStack[dependeeStoryNumber])
				return true;
		}
	}

	recStack[v] = false; // remove the story from recursion stack
	return false;
}

/*
// Adapted from:
// GeeksforGeeks. (2018). Detect Cycle in a Directed Graph - GeeksforGeeks. [online] Available at: https://www.geeksforgeeks.org/detect-cycle-in-a-graph/ [Accessed 12 Nov. 2018].
// This function is a variation of DFS() in https://www.geeksforgeeks.org/archives/18212 
*/
// Returns true if the DAG of dependencies between stories has a cycle, false if not
bool isCyclic(vector <Story> allStories) {
	// Mark all the stories as not visited and not part of recursion stack
	bool *visited = new bool[allStories.size()];
	bool *recStack = new bool[allStories.size()];

	for (int i = 0; i < allStories.size(); ++i) {
		visited[i] = false;
		recStack[i] = false;
	}

	// Call the recursive helper function on each story to detect cycles in different DFS trees
	for (int i = 0; i < allStories.size(); ++i) {
		if (_isCyclic(i, visited, recStack, allStories))
			return true;
	}

	return false;
}

// Returns a vector of Story objects filled with random values
vector<Story> randomlyGenerateStories(int numberOfStories, int minBusinessValue, int maxBusinessValue, int minStoryPoints, int maxStoryPoints) {
	vector<Story> storyData;

	// Geometric sequence of probabilities for the discrete distribution random number generator
	vector<double> probabilities = geometricSequence(0.5, 0.5, (double)numberOfStories);

	// Create stories with random values
	for (int i = 0; i < numberOfStories; ++i) {
		int businessValue = randomInt(minBusinessValue, maxBusinessValue);
		int storyPoints = randomInt(minStoryPoints, maxStoryPoints);

		storyData.push_back(Story(i, businessValue, storyPoints));
	}

	for (int i = 0; i < numberOfStories; ++i) {
		// The maximum number of dependencies that story i can have
		// (can't force a specific number of dependencies as it might create cycles in the graph of dependencies between stories)
		int maxNumberOfDependencies = randomIntDiscreteDistribution(probabilities);

		for (int j = 0; j < maxNumberOfDependencies; ++j) {
			// Pick a random story as a potential dependency of story i
			int potentialDependee = randomInt(0, numberOfStories - 1);

			// Check if the dependency is the same as story i
			bool isSelfLoop = potentialDependee == i;

			// Check if the dependency is already a dependency of story i
			bool isAlreadyDependency = find(storyData[i].dependencies.begin(), storyData[i].dependencies.end(), potentialDependee) != storyData[i].dependencies.end();

			// Retry with another random story
			if (isSelfLoop || isAlreadyDependency) {
				--j;
				continue;
			}

			// Add the dependency to story i
			storyData[i].dependencies.push_back(potentialDependee);

			// Check if adding the dependency created a cycle in the graph of dependencies (which makes it unsolvable)
			bool dependencyCreatesCycle = isCyclic(storyData);
			
			if (dependencyCreatesCycle) {
				// Remove the offending dependency
				storyData[i].dependencies.pop_back();
			}
		}
	}

	return storyData;
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

int main(int argc, char* argv[]) {
	// Seed the random number generator
	srand(time(NULL));

	// The number of full time employees able to work on tasks (to estimate the sprint velocity)
	int numberOfFTEs = 5;

	// The number of sprints available in the roadmap
	int numberOfSprints;
	// Holds the data about each sprint
	vector<Sprint> sprintData;

	// The number of stories in the product backlog
	int numberOfStories;
	// Holds the data about each user story
	vector<Story> storyData;
	
	// If the user wants to see the solution
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

		// Generate some test data to optimise
		storyData = randomlyGenerateStories(numberOfStories, 1, 10, 1, 8);
		sprintData = randomlyGenerateSprints(numberOfSprints, 0, 8 * numberOfFTEs);
		
		break;
	default:
		// If no parameters are given, use some hard-coded test data
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

		// 2D array of Boolean decision variables (roadmap[i][j] == 1 means user story j is taken in sprint i)
		IloArray<IloBoolVarArray> roadmap(env, numberOfSprints);

		// Total business value delivered (across the whole roadmap)
		IloNumExpr deliveredValue(env, 0);

		for (int i = 0; i < numberOfSprints; ++i) {
			// Sum of the story points taken in the sprint i
			IloNumExpr storyPointsTaken(env, 0);

			// Sprint i is represented by an array of Boolean decision variables
			roadmap[i] = IloBoolVarArray(env, numberOfStories);

			for (int j = 0; j < numberOfStories; ++j) {
				// Create a Boolean decision variable
				roadmap[i][j] = IloBoolVar(env);

				// Add business value (including sprint bonus), if story is taken in this sprint
				deliveredValue += storyData[j].businessValue * sprintData[i].sprintValueBonus * roadmap[i][j];

				// Add story points, if story j is taken in sprint i
				storyPointsTaken += storyData[j].storyPoints * roadmap[i][j];
			}

			// Add constraint (per sprint): sum of story points taken in sprint i <= sprint i's capacity
			model.add(storyPointsTaken <= sprintData[i].sprintCapacity);
		}

		// Story is only assiged to one (or no) sprint
		for (int j = 0; j < numberOfStories; ++j) {
			// How many times story j is assigned in sprint i
			IloNumExpr numberOfTimesStoryIsUsed(env, 0);

			for (int i = 0; i < numberOfSprints; ++i) {
				numberOfTimesStoryIsUsed += roadmap[i][j];
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
						numberOfTimesDependeesPreAssigned += roadmap[sprintLookback][storyToCheck];
					}

					// roadmap[i][j] == 1 means that dependent story j is taken in sprint i, so dependee story d must appear in previous roadmap
					// roadmap[i][j] == 0 means that dependent story j is not taken in sprint i, so dependee story d may or may not appear in previous roadmap
					model.add(IloIfThen(env, roadmap[i][j] == 1, numberOfTimesDependeesPreAssigned == 1));

					// NOTE - is numberOfTimesDependeesPreAssigned == 1 (per dependee, per story) better than numberOfTimesDependeesPreAssigned == numberOfDependencies (per story)?
					// i.e. is it better to have more or fewer constraints that check the same thing?
				}
			}
		}

		// Objective function
		model.add(IloMaximize(env, deliveredValue));

		// Calculate the total size of the product backlog

		int maxBusinessValuePossible = 0;
		int maxStoryPointsPossible = 0;

		for (int i = 0; i < storyData.size(); ++i) {
			maxBusinessValuePossible += storyData[i].businessValue;
			maxStoryPointsPossible += storyData[i].storyPoints;
		}

		IloCplex cplex(model);

		if (cplex.solve()) {
			if (showSolution) {
				cout << endl << "Product backlog: " << endl << endl;

				for (int i = 0; i < storyData.size(); ++i) {
					cout << storyData[i].toString() << endl;
				}

				cout << endl << "Available sprints: " << endl << endl;

				for (int i = 0; i < sprintData.size(); ++i) {
					cout << sprintData[i].toString() << endl;
				}

				cout << endl << "---------------------------------------------------------" << endl << endl;

				int totalStoriesDelivered = 0;
				int totalBusinessValueDelivered = 0;
				int totalStoryPointsDelivered = 0;

				cout << "Sprint plan:" << endl << endl;

				// Output the solution
				for (int i = 0; i < numberOfSprints; ++i) {
					// Sprint information
					cout << sprintData[i].toString() << endl;
					int businessValueDelivered = 0;

					for (int j = 0; j < numberOfStories; ++j) {
						// If the story was taken in this sprint, print the story's information
						if (cplex.getValue(roadmap[i][j]) == 1) {
							businessValueDelivered += storyData[j].businessValue;

							// Running totals for how much the roadmap delivered overall
							totalBusinessValueDelivered += storyData[j].businessValue;
							totalStoriesDelivered += 1;
							totalStoryPointsDelivered += storyData[j].storyPoints;

							cout << "\t" << storyData[j].toString() << endl;
						}
					}

					// What was delivered in the sprint
					cout << "-- [Total delivered: " << to_string(businessValueDelivered) << " business value, "
						<< to_string(businessValueDelivered * sprintData[i].sprintValueBonus) << " weighted business value]" << endl << endl;
				}

				cout << endl << "Solution status: " << cplex.getStatus() << endl << endl;
				cout << "Stories delivered: " << totalStoriesDelivered << "/" << storyData.size() << " (" << 100.0 * (double)totalStoriesDelivered / (double)storyData.size() << "%)" << endl;

				cout << "Total business value delivered: " << totalBusinessValueDelivered << "/" << maxBusinessValuePossible
					<< " (" << 100.0 * (double)totalBusinessValueDelivered / (double)maxBusinessValuePossible << "%)" << endl;

				cout << "Total story points delivered: " << totalStoryPointsDelivered << "/" << maxStoryPointsPossible
					<< " (" << 100.0 * (double)totalStoryPointsDelivered / (double)maxStoryPointsPossible << "%)" << endl;

				cout << endl << "Total weighted business value: " << cplex.getObjValue() << endl << endl;
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