
#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include <fstream>
#include <string>
#include <optional>
#include <random>

constexpr int daysInCycle{ 5 }; //number of days in generated schedule
constexpr int periodsInDay{ 10 }; //number of periods (schedule slots) in each day


std::random_device rd{};
std::seed_seq ss{ rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd() }; //generates a seed sequence using the OS's random device
std::mt19937 mt{ ss }; //seeds merene twister using the generated seed sequence

class Staff; //staff class prototype so it can be referred to in Activity

class Activity
{

	std::string m_activityName{}; //the display name of the activity
	std::vector <std::size_t> m_timesAvailable{}; //holds the indices of the schedule slots where this activity can occur
	std::vector <std::size_t> m_scheduleTimesAvailable{}; 
	int m_timesPerCycle{}; //number of times this activity should occur in the generated schedule
	int m_offset{}; //the offset of the randomly generated ranges for selecting random schedule slots
	int m_activityID{}; //the unique id of the activity
	std::vector <Staff*> m_preferred{}; //a list of the staff who prefer to lead this activity
	std::vector <Staff*> m_neutral{}; //a list of the staff who are neutral towards leading this activity
	std::vector <Staff*> m_unpreferred{}; //a list of the staff who prefer not to lead this activity

public:

	Activity() = default; //a default constructor with no arguments

	//creates Activity using its display name, list of when it can occur, how many times it should happen and unique id.
	Activity(const std::string_view activityName, std::vector <std::size_t>& timesAvailable, const int timesPerCycle, const int activityID)
		: m_activityName{ activityName },
		m_timesAvailable{ std::move(timesAvailable) }, //times available is moved to save computing costs of copying the list
		m_timesPerCycle{ timesPerCycle },
		m_activityID{ activityID }
	{}


	void setOffset(std::vector <int>& timeSlots)
	{
		std::size_t timesAvailableIndex{ 0 };
		std::size_t timeSlotsIndex{ 0 };
		while ((timesAvailableIndex < m_timesAvailable.size()) && (timeSlotsIndex < timeSlots.size()))
		{
			if (m_timesAvailable[timesAvailableIndex] == timeSlots[timeSlotsIndex])
			{
				m_scheduleTimesAvailable.push_back(m_timesAvailable[timesAvailableIndex]);
				++timesAvailableIndex;
				++timeSlotsIndex;
			}
			else if (m_timesAvailable[timesAvailableIndex] < timeSlots[timeSlotsIndex])
				++timesAvailableIndex;
			else
				++timeSlotsIndex;

		}
		int endOfRand{ 0 };
		if (m_timesPerCycle > 0)
			endOfRand = static_cast<int>(m_scheduleTimesAvailable.size()) % m_timesPerCycle;
		std::uniform_int_distribution randOffset{ 0, endOfRand };
		m_offset = randOffset(mt);
	}

	constexpr int getOffset() const
	{
		return m_offset;
	}

	constexpr std::vector <std::size_t> getTimesAvailable() const
	{
		return m_timesAvailable;
	}
	constexpr std::size_t getTimesAvailable(std::size_t index) const
	{
		return m_timesAvailable[index];
	}

	//returns total time available
	constexpr std::size_t getTotalTimesAvailable() const
	{
		return m_timesAvailable.size();
	}
	constexpr std::string_view getName() const
	{
		return m_activityName;
	}
	constexpr int getTimesPerCycle() const
	{
		return m_timesPerCycle;
	}
	constexpr int getActivityID() const
	{
		return m_activityID;
	}

};


class ActivityCategory
{
	std::string m_activityCategoryName{};
	std::vector<Activity> m_activities{};
	int m_timesPerCycle{};
	int m_categoryIndex{ 0 };
	int m_activityCounter{ -1 };


public:

	//creates ActivityCategory
	ActivityCategory(std::string_view activityCategoryName, std::vector<Activity>& activities, int timesPerCycle)
		:m_activities{ std::move(activities) },
		m_activityCategoryName{ activityCategoryName },
		m_timesPerCycle{ timesPerCycle },
		m_categoryIndex{ 0 },
		m_activityCounter{ -1 }
	{

		//sorts activities in category by their availibity
		std::sort(m_activities.begin(), m_activities.end(),
			[](Activity& a1, Activity& a2)
			{
				if (a1.getTotalTimesAvailable() < a2.getTotalTimesAvailable())
					return true;
				return false;
			});
	}
	void incActivityCounter()
	{
		++m_activityCounter;
	}
	Activity* getNextActivity()
	{
		std::cerr << 'r' << m_activityCounter;
		return  &(getActivities()[m_activityCounter]);
	}

	constexpr int getTimesPerCycle() const
	{
		return m_timesPerCycle;
	}
	constexpr std::string_view getName() const
	{
		return m_activityCategoryName;
	}
	constexpr std::vector<Activity>& getActivities()
	{
		return m_activities;
	}

	constexpr int getCategoryIndex() const
	{
		return m_categoryIndex;
	}

	void incCategoryIndex()
	{
		++m_categoryIndex;
	}

};


class ScheduleSlot
{
	std::vector<Activity*> m_possibleActivities{};
	//Group* group{};
	std::vector <Staff*> m_preferred{};
	std::vector <Staff*> m_neutral{};
	std::vector <Staff*> m_unpreferred{};
	Activity* m_activity{ nullptr };
	ActivityCategory* m_activityCategory{ nullptr };
	int m_id;

public:
	static int id;

	ScheduleSlot()
		:m_id{ id }
	{
		++id;
	}


	void addActivityCategory(ActivityCategory& cat)
	{
		m_activityCategory = &cat;
		m_activity = cat.getNextActivity();
		(*m_activityCategory).incCategoryIndex();
	}

	constexpr Activity* getActivity() const
	{
		return m_activity;
	}

	constexpr ActivityCategory* getActivityCategory() const
	{
		return m_activityCategory;
	}

	constexpr int getID() const
	{
		return id;
	}

};

//stores staff members
class Staff
{
	std::string m_name{}; //staff name
	std::vector<ScheduleSlot*> m_timesAvailableToLead{}; //holds pointers to schedule slots where this staff is available to lead (has not been booked and does not havve break or personal time)
	
	//stores staffs leading preferences
	std::vector <Activity*> m_preferred{}; //holds pointers to activities that this staff would prefer to lead
	std::vector <Activity*> m_neutral{};  //holds pointers to activities that this staff feels neutral about leading
	std::vector <Activity*> m_unpreferred{}; //holds pointers to activities that this staff would not prefer to, but can lead

public:

	//Staff constructor, memberwise initialization of all member variables
	Staff(const std::string_view name, const std::vector<ScheduleSlot*> timesAvailable, const std::vector <Activity*> preferred, const std::vector <Activity*> neutral, const std::vector <Activity*> unpreferred)
		: m_name{name},
		m_timesAvailableToLead{ timesAvailable },
		m_preferred{ preferred },
		m_neutral{ neutral },
		m_unpreferred{ unpreferred }
	{}
};




















class ParticipantGroup
{
	int m_participants{};
	std::vector <int> m_timeSlots{};
	int m_totalTimeSlots{};


public:

	static std::array<ScheduleSlot, daysInCycle* periodsInDay> scheduleSlots;

	ParticipantGroup(const int participants, std::vector <int>& timeSlots, const int totalTimeSlots)
		:m_participants{ participants },
		m_timeSlots{ std::move(timeSlots) },
		m_totalTimeSlots(totalTimeSlots)
	{
	}

	constexpr int getTotalTimeSlots() const
	{
		return m_totalTimeSlots;
	}

	//returns whether a given range is filled s
	bool rangeIsFilled(const int begin, const int end, const int range, std::vector<int>& slotsAvailable)
	{
		auto iterator{ scheduleSlots.begin() + begin };
		while (iterator < scheduleSlots.begin() + end)
		{

			if (iterator->getActivity() == nullptr)
				slotsAvailable.push_back((iterator)-scheduleSlots.begin());
			++iterator;
		}
		if (slotsAvailable.size() == 0)
		{
			std::cerr << "i";  //for bug testing

			return true;
		}

		return false;
	}

	//return index of random empty slot within range
	int findEmptySlotInRange(const int range, const int idealSlots, const Activity* activity, std::vector<int>& slotsAvailable)
	{
		int slotIndex;
		int endRand{ 0 };
		if (activity->getTimesPerCycle() > 0)
			endRand = static_cast<int>((idealSlots - activity->getOffset()) / activity->getTimesPerCycle());
		std::uniform_int_distribution randInRange{ 0,endRand }; //random spot within range
		do
		{

			std::uniform_int_distribution randslotAvailable{ 0,static_cast<int>(slotsAvailable.size() - 1) };
			slotIndex = activity->getTimesAvailable()[slotsAvailable[randslotAvailable(mt)]]; //chooses new schedule slot

		} while (scheduleSlots[slotIndex].getActivityCategory() != nullptr);// find new value while schedule slot is already filled
		return slotIndex;
	}

	//finds an appropriate slot to add teh activity category to
	int findSlot(const int idealSlots, const int offset, std::vector <int>& doneRanges, std::uniform_int_distribution<int>randRange, const std::vector < std::size_t >& unfilledSlots, const Activity* activity)
	{
		while (true) //loops until appropriate slot is found
		{
			int range = randRange(mt); //finds random range

			auto found = std::find(doneRanges.begin(), doneRanges.end(), range);
			if (found == doneRanges.end()) //if range does not already contain activity category
			{
				int begin{ 0 };//starts at 0 if first category range prevent going out of bounds

				int rangeEnd{ (range + 1) * (m_totalTimeSlots - offset) / idealSlots };

				int end{ std::min(rangeEnd,  static_cast<int>(activity->getTotalTimesAvailable())) }; //find end of range

				doneRanges.push_back(range); //adds range to filled list 

				std::vector<int> slotsAvailable{};
				if (!rangeIsFilled(begin, end, range, slotsAvailable))//if range is not filled and is available
				{
					return findEmptySlotInRange(range, idealSlots, activity, slotsAvailable); //adds to given range
				}

			}
			else if (idealSlots <= doneRanges.size()) //if all (or all but one) ranges are full
			{

				std::uniform_int_distribution randSlot{ 0,static_cast<int>(unfilledSlots.size() - 1) };
				return static_cast<int>(unfilledSlots[randSlot(mt)]); //returns a random empty slot
			}

		}

	}

	void assignActivityCategory(const int idealSlots, std::vector <std::vector<int>>& finishedRanges, ActivityCategory& activityCategory, std::vector < std::size_t >& unfilledSlots)
	{
		//loops until all of activity categories spots have been filled
		for (int activities{ 0 }; activities < static_cast<int>(activityCategory.getActivities().size()); ++activities) //fill a total of idealSlots slots
		{
			std::cerr << '?' << activities << ' ' << static_cast<int>(activityCategory.getActivities().size()) << "?\n";
			activityCategory.incActivityCounter();
			Activity* activity{ activityCategory.getNextActivity() };

			activity->setOffset(m_timeSlots);

			std::uniform_int_distribution randRange{ 0,idealSlots }; //random range

			//fills activity category
			int activityId{ activity->getActivityID() };
			int activitySlotsLeft{ activity->getTimesPerCycle() - static_cast<int>(finishedRanges[activityId].size()) };


			for (int idealActivitySlotsLeft{ activitySlotsLeft }; idealActivitySlotsLeft > 0; --idealActivitySlotsLeft)
			{
				int slotIndex{ findSlot(idealSlots, activity->getOffset(), finishedRanges[activityId], randRange, unfilledSlots, activity) }; //chooses a slot
				scheduleSlots[slotIndex].addActivityCategory(activityCategory); //adds activity category to chosen schedule slot
				unfilledSlots.erase((std::find(unfilledSlots.begin(), unfilledSlots.end(), slotIndex))); //removes slot from unfilled slots vector
			}
			int a{ 4 };

		}
	}

	//fills all the schedule slots in this participant group
	void fillAllSlots(std::vector <ActivityCategory>& categories, std::vector < std::size_t >& unfilledSlots, std::vector <std::vector<int>>& finishedRanges)
	{
		std::size_t catIndex{ 0 };
		while (unfilledSlots.size() > 0 && catIndex < categories.size()) //loops until all slots are filled
		{
			//slots left to fill for activity category in this participant group is total slots for activity category multiplied by % of total time slots in this participant group
			double percentOfTotal{ (static_cast<double>(m_totalTimeSlots) / (periodsInDay * daysInCycle)) };
			int idealSlots{ static_cast<int>(categories[catIndex].getTimesPerCycle() * percentOfTotal) };



			assignActivityCategory(idealSlots, finishedRanges, categories[catIndex], unfilledSlots);
			++catIndex; //iterates activity category
		}
	}

	//finds slots already filled prior to this participant group
	void findAlreadyFilledSlots(std::vector <ActivityCategory>& categories, std::vector < std::size_t >& unfilledSlots, std::vector <std::vector<int>>& finishedRanges)
	{

		for (std::size_t index{ 0 }; index < m_timeSlots.size(); ++index) //loops through all schedule slots in the participant gorup
		{
			if (scheduleSlots[index].getActivityCategory() == nullptr)
				unfilledSlots.push_back(index); //adds slot to unfilled list if slot is unfilled

			else //if slot is filled
			{
				int64_t activityCategory{ std::find_if(categories.begin(),categories.end(),[&](ActivityCategory cat) //finds activity category in schedule slot
					{
						if (cat.getName() == scheduleSlots[index].getActivityCategory()->getName())
							return true;
						return false;
					}) - categories.begin() };

				int64_t activity{ std::find_if(categories[activityCategory].getActivities().begin(),categories[activityCategory].getActivities().begin(),[&](Activity act) //finds activity category in schedule slot
					{
						if (act.getName() == scheduleSlots[index].getActivity()->getName())
							return true;
						return false;
					}) - categories[activityCategory].getActivities().begin() };

				//adds already filled range to array corresponding to it's activity category
				finishedRanges[categories[activityCategory].getActivities()[activity].getActivityID()].push_back(index / categories[activityCategory].getActivities()[activity].getTimesPerCycle());
			}
		}

	}

	//prints out cyclical schedule
	void printCycleSchedule()
	{
		for (std::size_t j{ 0 }; j < daysInCycle; ++j) //for each day in cycle
		{
			for (std::size_t i{ 0 }; i < periodsInDay; ++i) //for each period in day, print the schedule slot
				std::cout << i + 1 << ". " << scheduleSlots[j * 10 + i].getActivityCategory()->getName() << " " << scheduleSlots[j * 10 + i].getActivity()->getName() << "\n";
			std::cout << "---------------------------\n"; //divider between days
		}
	}

	//fills this participant group's schedule with activities
	void addActivities(std::vector <ActivityCategory>& categories, const int maxID)
	{
		std::vector <std::vector<int>> finishedRanges(maxID + 1); //stores which ranges are already filled with each activity category
		std::vector < std::size_t > unfilledSlots{}; //stores slots which are unfilled

		findAlreadyFilledSlots(categories, unfilledSlots, finishedRanges); //finds slots already filled prior to this participant group

		fillAllSlots(categories, unfilledSlots, finishedRanges); //fills all the schedule slots in this participant group

		printCycleSchedule(); //prints out cyclical schedule

		return;
	}

};



//find next semicolon in given string
std::size_t findNextSemi(std::string_view string)
{
	return string.find(':');
}

//find next comma in given string
std::size_t findEnd(std::string_view string)
{
	return string.find(',');
}


void getValues(std::string& line, std::vector<std::size_t>& fillVector)
{
	std::size_t(*endpoint)(std::string_view)(&findNextSemi);
	bool loopAgain{ true };
	while (loopAgain)	//loops while more time available ranges exist (while dividers exist plus once more)
	{
		if (line.find(':') == std::string::npos)//if range divider does not exist, stop looping after this iteration and search for boundary to times per cycle instead of between ranges
		{
			loopAgain = false;
			endpoint = &findEnd;
		}


		std::size_t startRange{ static_cast<std::size_t>(std::stoi(line.substr(0, line.find('-')))) - 1 };//gets start of range
		std::size_t endRange{ static_cast<std::size_t>(std::stoi(line.substr(line.find('-') + 1,endpoint(line)))) - 1 }; //gets end of range
		for (std::size_t index{ startRange }; index <= endRange; ++index) //while within range update time availble to true and iterate total times available
		{
			fillVector.push_back(index);
		}
		line = line.substr(endpoint(line) + 1, line.size() - endpoint(line)); //remove range added from range list
	}
}

void getStrings(std::string_view line, std::vector<std::string>& fillVector)
{
	std::size_t(*endpoint)(std::string_view)(&findNextSemi);
	bool loopAgain{ true };
	while (loopAgain)	//loops while more time available ranges exist (while dividers exist plus once more)
	{
		if (line.find(':') == std::string::npos)//if range divider does not exist, stop looping after this iteration and search for boundary to times per cycle instead of between ranges
		{
			loopAgain = false;
			endpoint = &findEnd;
		}

		fillVector.emplace_back(line.substr(0, endpoint(line)));
		line = line.substr(endpoint(line) + 1, line.size() - endpoint(line)); //remove range added from range list
	}
}

//adds activity to activities array, returns times per cycle for activity
int addActivity(std::string line, std::vector <Activity>& activities, const int activityID)
{
	std::size_t comma{ line.find(',') }; //find break between activity name and activity times available
	std::string activityName{ line.substr(0,comma) };
	line = line.substr(comma + 1, line.size() - comma - 1); //removes activty name from line


	std::vector < std::size_t > timesAvailable{};//array storing if activity is available at each time slot


	getValues(line, timesAvailable);

	int timesPerCycle{ std::stoi(line) };

	activities.push_back(Activity(activityName, timesAvailable, timesPerCycle, activityID)); //add activity to activities array
	return timesPerCycle;
}

//add activity cateogry to categories vector
void createActivityCategory(std::vector <ActivityCategory>& categories, std::string_view category, std::vector<Activity>& activities, int timesPerCycle)
{
	std::sort(activities.begin(), activities.end(), [](Activity first, Activity second)
		{
			return first.getTotalTimesAvailable() < second.getTotalTimesAvailable();
		});
	categories.push_back(ActivityCategory(category, activities, timesPerCycle));
}

//takes in a lsit of activity anmes to search for and the list of activity categories and fills a list of pointers to those activities
void getActivities(const std::vector<std::string>& activityNames, std::vector<Activity*> &activities, std::vector <ActivityCategory>& categories)
{
	for (std::size_t i{ 0 }; i < activityNames.size(); ++i) //loops through all activities we are searching for
	{
		bool loopAgain{ true }; //control variable that allows the process to skip to searching for next activity once the activity has been found
		for (std::size_t j{ 0 }; j < categories.size() && loopAgain; ++j) //loops through activity categories while there are more to search and the activity has not yet been found
		{
			//allows us to only copy the list of activitivies int the activity category one time
			std::vector<Activity>*  categoryActivities{ &(categories[j].getActivities()) };  //holds a pointer to the list of activities in the activity category
			for (std::size_t k{ 0 }; k < categoryActivities->size() && loopAgain; ++k) //loops through activities while there are more to search and the activity has not yet been found
			{
				
				if ((*categoryActivities)[k].getName() == activityNames[i]) //if the activity is the activity we are searching for (it has the same name as the next activity in the list of activities we are searching for)
				{
					activities.push_back(&(*categoryActivities)[k]); //add a pointer to the activity to the activities vector
					loopAgain = false; //marks activity as found, allowing process to skip to next activity
				}
			}
		}
	}
}

std::array<ScheduleSlot, daysInCycle* periodsInDay> ParticipantGroup::scheduleSlots{};

void getScheduleSlots(const std::vector<std::size_t> avail, std::vector<ScheduleSlot*> timesAvailable)
{
	for (std::size_t i{ 0 }; i < avail.size(); ++i)
	{
		timesAvailable.push_back(&ParticipantGroup::scheduleSlots[i]);
	}
}

//takes in a string and a breakpoint and fiils the inputted activity pointers vector with activity pointers to the activites found within the string
void processActivitiesListFromFileToVectorofActivityPointers(const std::string_view line, std::vector<Activity*> &activityPointers, std::vector <ActivityCategory>& categories, const std::size_t breakLocation)
{
	std::string raw{ line.substr(0,breakLocation) };  //hold raw list of activities
	std::vector<std::string> names{}; //holds list of activities names
	getStrings(raw, names); //processes raw list and fills list of activities names
	getActivities(names, activityPointers, categories); //gets list of pointers to activities using their names and fills activity pointers vector
}

void readInStaff(std::ifstream& myReader, std::vector <ActivityCategory>& categories)
{
	std::string line{};//holds line data
	while (std::getline(myReader, line)) //iterates for each staff in the file
	{
		std::size_t breakLocation{ line.find(',') };//location of break between staff name and preferred lead activities
		std::string name{ line.substr(0,breakLocation) }; //holds staff name



		line = line.substr(breakLocation + 1, line.size()); //line removes staff name and break
		std::vector<Activity*> preferred{}; //holds list of pointers to preffered activites
		breakLocation = line.find(','); //location of next breakpoint (at the end of the list of preffered activites)
		processActivitiesListFromFileToVectorofActivityPointers(line, preferred, categories, breakLocation); //fills preferred vector with pointer to activities between teh previous and current breakpoints

		line = line.substr(breakLocation + 1, line.size()); //line removes list of preferred names and break
		std::vector<Activity*> neutral{}; //holds list of pointers to preffered activites
		breakLocation = line.find(','); //location of next breakpoint (at the end of the list of neutral activites)
		processActivitiesListFromFileToVectorofActivityPointers(line, neutral, categories, breakLocation); //fills neutral vector with pointer to activities between teh previous and current breakpoints

		line = line.substr(breakLocation + 1, line.size()); //line removes staff name and first break
		std::vector<Activity*> unpreferred{}; //holds list of pointers to preffered activites
		breakLocation = line.find(','); //location of next breakpoint (at the end of the list of unpreferred activites)
		processActivitiesListFromFileToVectorofActivityPointers(line, unpreferred, categories, breakLocation); //fills unpreferred vector with pointer to activities between teh previous and current breakpoints

		breakLocation = line.find(',');
		line = line.substr(breakLocation + 1, line.size());
		std::vector<std::size_t> breaks{};
		getValues(line, breaks);
		std::size_t j{ 0 };
		std::vector<std::size_t> avail{};
		for (std::size_t i{ 0 }; i < periodsInDay * daysInCycle; ++i)
		{
			if (i != breaks[j])
				avail.push_back(i);
			else if (j < breaks.size() - 1)
				++j;
		}
		std::vector<ScheduleSlot*> timesAvailable{};
		getScheduleSlots(avail, timesAvailable);
		std::cout << preferred[0]->getName();
	}
}

//reads in activity and activity category info and stores it in categories vector
int readInActivityCategories(std::vector <ActivityCategory>& categories)
{
	std::ifstream myReader{ "scheduling.csv" }; //selects file "scheduling.csv" to read in
	int activityID{ 0 }; //sets next activity id to 0
	try
	{
		if (!myReader) //if reader fails to open file throw exception
			throw "File could not be opened\n";

		std::string line{};//holds line data
		std::getline(myReader, line); //skips first line (column headers)
		std::string prevCategory{ "" }; //holds previous activity category name



		std::vector <Activity> activities{};//vector storing activities belonging to activity group
		int timesPerCycle{ 0 }; //holds total number of times activity category will occur
		while (true) //loops until broken (when staff starts to be read in), reasds one activity at a time ine
		{
			std::getline(myReader, line); //gets line
			std::size_t comma{ line.find(',') };//location of break between category name and activity name
			std::string category{ line.substr(0,comma) }; //category name

			if (category == "Staff") //once staff is hit breaks and starts to read in staff
				break;

			if (category != prevCategory && prevCategory != "") //if category is new (not first category)
			{
				createActivityCategory(categories, prevCategory, activities, timesPerCycle); //create previous activity category
				timesPerCycle = 0; //resets number of times avctivity cateogry will occur
				activities.clear(); //reset activities
			}
			prevCategory = category; //ensures category is updated (important for first iteration)
			timesPerCycle += addActivity(line.substr(comma + 1, line.size() - comma - 1), activities, activityID); //adds activity using line excluidng activity category information
			++activityID; //iterates activity id
		}
		createActivityCategory(categories, prevCategory, activities, timesPerCycle); //creates final activity category
		readInStaff(myReader, categories); //reads in staff
	}
	catch (const char* errorMessage) //if file could not be opened
	{
		std::cerr << errorMessage; //print file error message
		throw; //rethrow exception
	}

	return activityID; //retuirns the file activityid (total number of activities)
}

//std::array<ScheduleSlot, daysInCycle* periodsInDay> ParticipantGroup::scheduleSlots{};
int ScheduleSlot::id{ 0 };

int main()
{



	std::vector <int> timeSlots{};

	for (int i{ 0 }; i < 50; ++i)
		timeSlots.push_back(i);

	//ParticipantGroup testGroup{ 1,timeSlots,50 };


	std::vector <ActivityCategory> categories{};

	int maxID{};

	try
	{
		maxID = readInActivityCategories(categories);
	}
	catch (...)
	{
		std::cerr << "A fatal error has occured\n";
	}

	std::sort(categories.begin(), categories.end(), [](ActivityCategory first, ActivityCategory second)
		{
			return first.getTimesPerCycle() < second.getTimesPerCycle();
		});

	//testGroup.addActivities(categories, maxID);

	std::cerr << "1";
	/*std::vector <ActivityCategory> ActivityCategories{};



	std::vector<ParticipantGroup> participantGroups{};
	std::vector <int> timeSlot{ 0,1,2,3 };
	participantGroups.emplace_back(50, timeSlot, 20);


	std::sort(participantGroups.begin(), participantGroups.end(),
		[](ParticipantGroup &s1, ParticipantGroup &s2)
		{
			if (s1.getTotalTimeSlots() < s2.getTotalTimeSlots())
				return true;
			return false;
		});

	for (const auto& e : participantGroups)
	{
		std::cout << e.getTotalTimeSlots() << '\n';
	}
	*/
}
