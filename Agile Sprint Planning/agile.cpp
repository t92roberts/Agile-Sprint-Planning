#include <ilcplex/ilocplex.h>
#include <vector>
#include <string>
#include <algorithm>
#include <set>
#include <tuple>
#include <chrono>
#include <map>
ILOSTLBEGIN

using namespace std;

class Story {
public:
	int storyNumber, businessValue, storyPoints;
	vector<int> dependencies;
	vector<int> dependees;

	Story() {};

	Story(int storyNumber, int businessValue, int storyPoints) {
		this->storyNumber = storyNumber;
		this->businessValue = businessValue;
		this->storyPoints = storyPoints;
	}

	Story(int storyNumber, int businessValue, int storyPoints, vector<int> dependencies) {
		this->storyNumber = storyNumber;
		this->businessValue = businessValue;
		this->storyPoints = storyPoints;
		this->dependencies = dependencies;
	}

	string printDependencies() {
		if (this->dependencies.size() > 0) {
			string dependenciesString = "";

			for (int i = 0; i < this->dependencies.size(); ++i) {
				if (i == 0)
					dependenciesString += "Story " + to_string(this->dependencies[i]);
				else
					dependenciesString += ", Story " + to_string(this->dependencies[i]);
			}

			return dependenciesString;
		}
		else
			return "None";
	}

	string printDependees() {
		if (this->dependees.size() > 0) {
			string dependeesString = "";

			for (int i = 0; i < this->dependees.size(); ++i) {
				if (i == 0)
					dependeesString += "Story " + to_string(this->dependees[i]);
				else
					dependeesString += ", Story " + to_string(this->dependees[i]);
			}

			return dependeesString;
		}
		else
			return "None";
	}

	bool operator == (const Story& other) const {
		return this->storyNumber == other.storyNumber;
	}

	bool operator != (const Story& other) const {
		return this->storyNumber != other.storyNumber;
	}

	bool operator < (const Story& other) const {
		return this->storyNumber < other.storyNumber;
	}

	bool operator <= (const Story& other) const {
		return this->storyNumber <= other.storyNumber;
	}

	string toString() {
		return "Story " + to_string(storyNumber)
			+ " (business value: " + to_string(businessValue)
			+ " | story points: " + to_string(storyPoints)
			+ " | dependencies: " + printDependencies() + ")";
	}
};

class Sprint {
public:
	int sprintNumber, sprintCapacity, sprintBonus;

	Sprint() {};

	Sprint(int num, int cap, int bonus) {
		this->sprintNumber = num;
		this->sprintCapacity = cap;
		this->sprintBonus = bonus;
	}

	bool withinCapacity(int storyPoints) {
		return storyPoints <= this->sprintCapacity;
	}

	bool operator == (const Sprint& other) const {
		return this->sprintNumber == other.sprintNumber;
	}

	bool operator < (const Sprint& other) const {
		return this->sprintNumber < other.sprintNumber;
	}

	bool operator <= (const Sprint& other) const {
		return this->sprintNumber <= other.sprintNumber;
	}

	string toString() {
		string sprint;

		if (sprintNumber != -1)
			sprint = ">> Sprint " + to_string(sprintNumber);
		else
			sprint = ">> Product Backlog";

		return sprint +
			" (capacity: " + to_string(sprintCapacity) +
			", bonus: " + to_string(sprintBonus) + ")";
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

class Roadmap {
public:
	vector<Story> stories;
	vector<Sprint> sprints;

	map<Story, Sprint> storyToSprint;
	map<Sprint, vector<Story>> sprintToStories;

	Roadmap() {};

	Roadmap(vector<Story> stories, vector<Sprint> sprints) {
		this->stories = stories;
		this->sprints = sprints;
	}

	bool validInsert(Story story, Sprint sprint) {
		// Check if adding the story overloads the sprint
		if (story.storyPoints + storyPointsAssignedToSprint(sprint) > sprint.sprintCapacity && sprint.sprintNumber != -1)
			return false;

		// Check that no dependees are assigned earlier/same as the sprint
		for (int dependeeNumber : story.dependees) {
			Story dependee = stories[dependeeNumber];

			auto it = storyToSprint.find(dependee);

			// The dependee is assigned somewhere
			if (it != storyToSprint.end()) {
				Sprint dependeeAssignedSprint = it->second;

				// The dependee is assigned earlier/same as this sprint
				if (dependeeAssignedSprint.sprintNumber <= sprint.sprintNumber && dependeeAssignedSprint.sprintNumber != -1)
					return false;
			}
		}

		// Check that each of the story's dependencies are assigned before the sprint
		for (int dependencyNumber : story.dependencies) {
			Story dependency = stories[dependencyNumber];

			auto it = storyToSprint.find(dependency);

			// Only stories that are assigned to a sprint are in the map
			if (it == storyToSprint.end())
				// The dependee isn't assigned to a sprint
				return false;

			Sprint dependencyAssignedSprint = it->second;

			// The dependee is assigned to the product backlog
			if (dependencyAssignedSprint.sprintNumber == -1)
				return false;

			// The story is assigned to an earlier sprint than its dependee
			if (sprint.sprintNumber <= dependencyAssignedSprint.sprintNumber)
				return false;
		}

		// The sprint doesn't get overloaded and the story's dependencies are satisfied
		return true;
	}

	int storyPointsAssignedToSprint(Sprint sprint) {
		vector<Story> sprintStories = sprintToStories[sprint];

		// Sum of the story points of all the stories assigned to the sprint
		int assignedStoryPoints = 0;

		for (Story story : sprintStories)
			assignedStoryPoints += story.storyPoints;

		return assignedStoryPoints;
	}

	void addStoryToSprint(Story story, Sprint sprint) {
		storyToSprint[story] = sprint;
		sprintToStories[sprint].push_back(story);
	}

	void removeStoryFromSprint(Story story, Sprint sprint) {
		storyToSprint.erase(storyToSprint.find(story));
		sprintToStories[sprint].erase(remove(sprintToStories[sprint].begin(), sprintToStories[sprint].end(), story), sprintToStories[sprint].end());
	}

	void moveStory(Story story, Sprint from, Sprint to) {
		removeStoryFromSprint(story, from);
		addStoryToSprint(story, to);
	}

	int calculateValue() {
		int totalValue = 0;

		for (pair<Story, Sprint> pair : storyToSprint) {
			Story story = pair.first;
			Sprint sprint = pair.second;

			// Don't add value from stories assigned to the product backlog
			if (sprint.sprintNumber != -1)
				totalValue += story.businessValue * sprint.sprintBonus;
		}

		return totalValue;
	}

	bool sprintCapacitiesSatisifed() {
		for (pair<Sprint, vector<Story>> pair : sprintToStories) {
			Sprint sprint = pair.first;

			// Don't check the capacity of the product backlog
			if (sprint.sprintNumber != -1) {
				int assignedStoryPoints = storyPointsAssignedToSprint(sprint);

				// Check if the sprint is overloaded
				if (!sprint.withinCapacity(assignedStoryPoints))
					// The story points assigned are not within the sprint's capacity
					return false;
			}
		}

		// All sprints are within capacity
		return true;
	}

	bool storyDependenciesSatisfied() {
		for (pair<Story, Sprint> assignment : storyToSprint) {
			Story story = assignment.first;
			Sprint assignedSprint = assignment.second;

			// Don't check stories assigned to the product backlog
			if (assignedSprint.sprintNumber != -1) {
				for (int dependeeNumber : story.dependencies) {
					Story dependee = stories[dependeeNumber];
					Sprint dependeeAssignedSprint = storyToSprint[dependee];

					// Only stories that are assigned to a sprint are in the map
					if (storyToSprint.find(dependee) == storyToSprint.end()) {
						// The dependee isn't assigned to a sprint
						return false;
					}
					else if (dependeeAssignedSprint.sprintNumber == -1) {
						// The dependee is assigned to the special product backlog
						return false;
					}
					else {
						// Check where the story is assigned compared to its dependee
						if (assignedSprint <= dependeeAssignedSprint)
							// The story is assigned to an earlier sprint than its dependee
							return false;
					}
				}
			}
		}

		// All stories have their dependees assigned to an earlier sprint
		return true;
	}

	bool isFeasible() {
		return sprintCapacitiesSatisifed() && storyDependenciesSatisfied();
	}

	string printStoryRoadmap() {
		string outputString = "";

		for (pair<Story, Sprint> pair : storyToSprint) {
			Story story = pair.first;
			Sprint sprint = pair.second;

			outputString += story.toString() + "\n  >> " + sprint.toString() + "\n";
		}

		return outputString;
	}

	string printSprintRoadmap() {
		string outputString = "";

		for (Sprint sprint : sprints) {
			if (sprint.sprintNumber == -1)
				outputString += "Product Backlog";
			else
				outputString += sprint.toString();

			vector<Story> sprintStories = sprintToStories[sprint];
			int valueDelivered = 0;
			int storyPointsAssigned = 0;

			if (sprintStories.empty()) {
				outputString += "\n\tNone";
			}
			else {
				for (Story story : sprintStories) {
					valueDelivered += story.businessValue;
					storyPointsAssigned += story.storyPoints;

					outputString += "\n\t" + story.toString();
				}
			}

			outputString += "\n-- [Value: " + to_string(valueDelivered)
				+ " (weighted value: " + to_string(valueDelivered * sprint.sprintBonus) + "), "
				+ "story points: " + to_string(storyPointsAssigned) + "]";

			outputString += "\n\n";
		}

		return outputString;
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

vector<string> splitString(const string& s, char delimiter) {
	vector<string> tokens;
	string token;
	istringstream tokenStream(s);

	while (getline(tokenStream, token, delimiter)) {
		tokens.push_back(token);
	}

	return tokens;
}

Roadmap greedyInsertStories(vector<Story> storiesToInsert, Roadmap roadmap) {
	while (storiesToInsert.size() > 0) {
		Story story = storiesToInsert[0];

		// Greedily re-insert the story into a sprint
		for (Sprint sprint : roadmap.sprints) {
			if (sprint.sprintNumber == -1 || roadmap.validInsert(story, sprint)) {
				roadmap.addStoryToSprint(story, sprint);
				storiesToInsert.erase(remove(storiesToInsert.begin(), storiesToInsert.end(), story), storiesToInsert.end());

				// Break out of traversing the sprints and move to the next story
				break;
			}
		}
	}

	return roadmap;
}

Roadmap randomRoadmap(vector<Story> storyData, vector<Sprint> sprintData) {
	vector<Story> shuffledStories = storyData;
	random_shuffle(shuffledStories.begin(), shuffledStories.end());
	Roadmap roadmap = greedyInsertStories(shuffledStories, Roadmap(storyData, sprintData));
	return roadmap;
}

int main(int argc, char* argv[]) {
	// Seed the random number generator
	srand(time(NULL));

	vector<Story> storyData;
	string storyDataFileName;

	// Holds the data about each sprint
	vector<Sprint> sprintData;
	string sprintDataFileName;

	switch (argc) {
	case 3:
		storyDataFileName = argv[1];
		sprintDataFileName = argv[2];
		break;
	default:
		exit(0);
	}

	// Load story data into objects //////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////

	string line;
	ifstream storiesFile(storyDataFileName);

	getline(storiesFile, line); // Skip column headers

	vector<pair<int, string>> dependenciesStrings;

	if (storiesFile.is_open()) {
		while (getline(storiesFile, line)) {
			vector<string> splitLine = splitString(line, ',');

			int storyNumber = stoi(splitLine[0]);
			int businessValue = stoi(splitLine[1]);
			int storyPoints = stoi(splitLine[2]);

			storyData.push_back(Story(storyNumber, businessValue, storyPoints));

			if (splitLine.size() == 4) {
				string dependencyString = splitLine[3];

				vector<string> splitDependencies = splitString(dependencyString, ';');

				for (string dependeeString : splitDependencies) {
					storyData[storyNumber].dependencies.push_back(stoi(dependeeString));
				}
			}
		}

		for (Story story : storyData) {
			for (int dependency : story.dependencies) {
				storyData[dependency].dependees.push_back(story.storyNumber);
			}
		}
	}
	else {
		cout << "Cannot open story data file" << endl;
		exit(0);
	}

	storiesFile.close();

	// Load sprint data into objects /////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////

	ifstream sprintsFile(sprintDataFileName);

	getline(sprintsFile, line); // Skip column headers

	if (sprintsFile.is_open()) {
		while (getline(sprintsFile, line)) {
			vector<string> splitLine = splitString(line, ',');

			int sprintNumber = stoi(splitLine[0]);
			int sprintCapacity = stoi(splitLine[1]);
			int sprintBonus = stoi(splitLine[2]);

			sprintData.push_back(Sprint(sprintNumber, sprintCapacity, sprintBonus));
		}

		sprintData.push_back(Sprint(-1, 0, 0)); // A special sprint representing 'unassigned' (i.e. assigned to the product backlog)
	}
	else {
		cout << "Cannot open sprint data file" << endl;
		exit(0);
	}

	sprintsFile.close();

	auto t_solveStart = chrono::high_resolution_clock::now();

	int numberOfStories = storyData.size();
	int numberOfSprints = sprintData.size();

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
				deliveredValue += storyData[j].businessValue * sprintData[i].sprintBonus * roadmap[i][j];

				// Add story points, if story j is taken in sprint i
				storyPointsTaken += storyData[j].storyPoints * roadmap[i][j];
			}

			// If the sprint has stories assigned, make sure that the sprint is not overloaded
			model.add(IloIfThen(env, storyPointsTaken > 0, storyPointsTaken <= sprintData[i].sprintCapacity));
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

				// No dependency-checking constraints are added for stories with no dependencies
				for (int d = 0; d < numberOfDependencies; ++d) {
					// The story number of the dependee story
					int storyToCheck = storyData[j].dependencies[d];

					// How many times the dependee stories havr been assigned before this sprint
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
					// It seems to run faster to have a constraint per dependee, per story
				}
			}
		}

		// Objective function
		model.add(IloMaximize(env, deliveredValue));

		IloCplex cplex(model);
		cplex.setOut(env.getNullStream());

		// CPLEX tuning
		// http://www-01.ibm.com/support/docview.wss?uid=swg21400023#Item6

		// Use MIP start to help the B&B by giving it a greedily-built solution
		// https://www.ibm.com/support/knowledgecenter/SSSA5P_12.8.0/ilog.odms.cplex.help/CPLEX/OverviewAPIs/topics/MIP_starts.html

		//////////////////////////////////////////////////////////////////////////

		Roadmap warmStart = randomRoadmap(storyData, sprintData);

		IloNumVarArray startVar(env);
		IloNumArray startVal(env);

		for (auto pair : warmStart.sprintToStories) {
			Sprint sprint = pair.first;

			if (sprint.sprintNumber != -1) {
				vector<Story> stories = pair.second;

				for (Story story : stories) {
					startVar.add(roadmap[sprint.sprintNumber][story.storyNumber]);
					startVal.add(1);
				}
			}
		}

		cplex.addMIPStart(startVar, startVal);

		startVal.end();
		startVar.end();
		
		//////////////////////////////////////////////////////////////////////////

		if (cplex.solve()) {
			auto t_solveEnd = chrono::high_resolution_clock::now();

			int totalStoriesDelivered = 0;
			int totalBusinessValueDelivered = 0;
			int totalStoryPointsDelivered = 0;

			// Output the solution
			for (int i = 0; i < numberOfSprints; ++i) {
				// Sprint information
				cout << sprintData[i].toString() << endl;

				int businessValueDelivered = 0;
				int storyPointedDelivered = 0;

				for (int j = 0; j < numberOfStories; ++j) {
					// If the story was taken in this sprint, print the story's information
					if (cplex.getValue(roadmap[i][j]) == 1) {
						businessValueDelivered += storyData[j].businessValue;
						storyPointedDelivered += storyData[j].storyPoints;

						// Running totals for how much the roadmap delivered overall
						totalBusinessValueDelivered += storyData[j].businessValue;
						totalStoryPointsDelivered += storyData[j].storyPoints;

						cout << "\t" << storyData[j].toString() << endl;
					}
				}

				// What was delivered in the sprint
				cout << "-- [Value: " << to_string(businessValueDelivered)
					<< " (weighted business value: " << to_string(businessValueDelivered * sprintData[i].sprintBonus) << "), "
					<< "story points: " << to_string(storyPointedDelivered) << "]" << endl << endl;
			}

			cout << endl << cplex.getStatus() << endl;
			cout << "Solved in " << chrono::duration<double, std::milli>(t_solveEnd - t_solveStart).count() << " ms" << endl << endl;
			cout << "Stories: " << storyData.size() << ", sprints: " << sprintData.size() << endl;
			cout << "Total weighted business value: " << cplex.getObjValue() << endl << endl;
			cout << "----------------------------------------" << endl;

			//cout << endl << storyData.size() << "," << sprintData.size() - 1 << "," << set << ",cold";
			//cout << "," << cplex.getObjValue() << "," << chrono::duration<double, std::milli>(t_solveEnd - t_solveStart).count();
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