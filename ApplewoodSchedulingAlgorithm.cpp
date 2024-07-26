
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
class ActivityCategory; //Activity Category class prototype so it can be referred to in Activity

class ActivityAndScheduleSlotWrapper
{
public:

	//made int to prevent underflow errors in its children classes
	virtual constexpr int getNumberToDiscard() const = 0; //gets number of options to be discarded before filling the spot
	virtual const int getAvailableStaffToLead() const = 0; //gets staff leaders available to fill this spot
	virtual constexpr int getSpotsLeftToFill() const = 0; //spots left to fill
};

//Represents each activity
class Activity :public ActivityAndScheduleSlotWrapper
{
	ActivityCategory* m_activityCategory{ nullptr }; //holds a pointer to the activity category which this activity belongs to

	std::string m_activityName{}; //the display name of the activity
	std::vector <std::size_t> m_timesAvailable{}; //holds the indices of the schedule slots where this activity can occur
	std::vector <std::size_t> m_scheduleTimesAvailable{}; 
	int m_timesPerCycle{}; //number of times this activity should occur in the generated schedule
	int m_timesLeftPerCycle{}; //how many more of this activity should occur
	int m_offset{}; //the offset of the randomly generated ranges for selecting random schedule slots

	//note: it is up to the intializer to ensure that the id of the activity is unique 
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
		m_timesLeftPerCycle{ timesPerCycle },
		m_activityID{ activityID }
	{}

	//long term should likely be removed
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

	//gets a copy of the offset
	constexpr int getOffset() const
	{
		return m_offset;
	}

	//gets timesAvailable array
	const std::vector <std::size_t>& getTimesAvailable() const 
	{
		return m_timesAvailable;
	}

	//gets a specific index of the timesAvailable array
	constexpr std::size_t getTimesAvailable(const std::size_t index) const
	{
		return m_timesAvailable[index];
	}

	//gets the number of elements in the timesAvailable array
	constexpr std::size_t getTotalTimesAvailable() const
	{
		return m_timesAvailable.size();
	}

	//gets this activity's name
	constexpr std::string_view getName() const
	{
		return m_activityName;
	}

	//gets how many times this activity should occur
	constexpr int getTimesPerCycle() const
	{
		return m_timesPerCycle;
	}

	//gets the unique ID of this activity
	constexpr int getActivityID() const
	{
		return m_activityID;
	}

	//sets the activity category of this activity
	void setActivityCategory(ActivityCategory* activityCategory)
	{
		m_activityCategory = activityCategory;
	}

	//Discarded options in activity are spots - number of spots to fill
	constexpr int getNumberToDiscard() const
	{
		return static_cast<int>(m_timesAvailable.size())-m_timesLeftPerCycle+1;
	}

	//only staff listed in preferred, neutral or unpreferred are able to lead the activity
	const int getAvailableStaffToLead() const
	{
		return static_cast<int>(m_preferred.size() + m_neutral.size() + m_unpreferred.size());
	}

	//gets spots left to fill
	virtual constexpr int getSpotsLeftToFill() const
	{
		return m_timesLeftPerCycle;
	}

};


//Represents each cateogry of activities
class ActivityCategory
{
	std::string m_activityCategoryName{}; //the name of the activity category
	std::vector<Activity> m_activities{}; //a list of the activities contained within the category
	int m_timesPerCycle{ 0 }; //the number of times this activity category should appear
	std::size_t m_activityCounter{ 0 }; //holds index of next activity in category to fill


public:

	//creates ActivityCategory using its name and an array of its activities
	ActivityCategory(std::string_view activityCategoryName, std::vector<Activity>& activities)
		:m_activities{ std::move(activities) }, //activities are moved to save computing costs of copying the list
		m_activityCategoryName{ activityCategoryName },
		m_activityCounter{ 0 }
	{

		//sorts activities in category by their availibity, ascending
		std::sort(m_activities.begin(), m_activities.end(),
			[](Activity& a1, Activity& a2)
			{
				if (a1.getTotalTimesAvailable() < a2.getTotalTimesAvailable())
					return true;
				return false;
			});


		
		for (auto& activity : m_activities)
		{
			m_timesPerCycle += activity.getTimesPerCycle(); //calculates how many times the category should occur as a sum of how many times its activities occur
			activity.setActivityCategory(this); //sets each activity's activity category to this activity category
		}
	}

	//long term this should probably be removed
	//increments the activity counter
	constexpr void incActivityCounter()
	{
		++m_activityCounter;
	}

	//long term this should probably be removed
	//gets next activity to fill
	Activity* getNextActivity()
	{
		return  &(getActivities()[m_activityCounter]);
	}

	//gets a copy of  how many times this activity category should occur
	constexpr int getTimesPerCycle() const
	{
		return m_timesPerCycle;
	}

	//gets the name of the activity category
	constexpr std::string_view getName() const
	{
		return m_activityCategoryName;
	}

	//gets the array of activities belonging to thisa ctivity category
	constexpr std::vector<Activity>& getActivities()
	{
		return m_activities;
	}



};


//Represents each schedule time period
class ScheduleSlot : public ActivityAndScheduleSlotWrapper
{
	std::vector<Activity*> m_possibleActivities{}; //a list of all activities which can possibly occur in this slot
	std::vector <Staff*> m_availableToLead{}; //a list of staff members who are available to lead in this slot
	//Group* group{};
	Activity* m_activity{ nullptr }; //the activity which occurs in this slot
	ActivityCategory* m_activityCategory{ nullptr }; //the activity catepgry which occurs in this slot
	int m_id; //the unique id of this schedule slot

public:
	static int id; //holds id of next schedule slot

	//intializes schedule slot, assigns id and iterates next id
	ScheduleSlot()
		:m_id{ id }
	{
		++id;
	}

	//adds activity category to the slot
	void addActivityCategory(ActivityCategory& cat)
	{
		m_activityCategory = &cat;
		m_activity = cat.getNextActivity();
	}

	//gets a pointer to the activity in the slot
	constexpr Activity* getActivity() const
	{
		return m_activity;
	}

	//gets a pointer to the activity category in the slot
	constexpr ActivityCategory* getActivityCategory() const
	{
		return m_activityCategory;
	}

	//gets a copy of the id in the slot
	constexpr int getID() const
	{
		return id;
	}

	//adds a staff to the availableToLead array
	constexpr void addAvailableToLead(Staff* staff)
	{
		m_availableToLead.push_back(staff);
	}

	//Discarded options in schedule slot are possible activities to fill the slot -1
	constexpr int getNumberToDiscard() const
	{
		return static_cast<int>(m_possibleActivities.size())-1;
	}

	//gets copy of number of staff available to lead at this slot's time
	const int getAvailableStaffToLead() const
	{
		return static_cast<int>(m_availableToLead.size());
	}

	//schedule's only ever require 1 spot to fill (unless they are already filled)
	virtual constexpr int getSpotsLeftToFill() const
	{
		if (!m_activity) //if nullptr ==> activity slot hasn't been filled yet
			return 0;
		else
			return 1;
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
	Staff(const std::string_view name, std::vector<ScheduleSlot*> timesAvailable, const std::vector <Activity*> preferred, const std::vector <Activity*> neutral, const std::vector <Activity*> unpreferred)
		: m_name{name},
		m_timesAvailableToLead{ std::move(timesAvailable) },
		m_preferred{ preferred },
		m_neutral{ neutral },
		m_unpreferred{ unpreferred }
	{
		//adds this staff to avaialble the available to lead lsit of every slot it is available to lead in
		for (const auto& slot : timesAvailable)
			slot->addAvailableToLead(this);
	}
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
			activityCategory.incActivityCounter();

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
	std::size_t(*endpoint)(std::string_view)(&findNextSemi); //initializes endpoint function pointer and sets it to point to find next semi
	bool loopAgain{ true }; //controls whether or not the loop will continue iterating
	while (loopAgain)	//loops while more time available ranges exist (while dividers exist plus once more)
	{
		if (line.find(':') == std::string::npos)//if range divider does not exist, stop looping after this iteration and search for boundary to times per cycle instead of between ranges
		{
			loopAgain = false; //stop iterating (this is the last range)
			endpoint = &findEnd; //set endpoint to comma (since that is the divider between csv columns)
		}


		std::size_t startRange{ static_cast<std::size_t>(std::stoi(line.substr(0, line.find('-')))) - 1 };//gets start of range
		std::size_t endRange{ static_cast<std::size_t>(std::stoi(line.substr(line.find('-') + 1,endpoint(line)))) - 1 }; //gets end of range
		for (std::size_t index{ startRange }; index <= endRange; ++index) //while within range update time available to true and iterate total times available
		{
			fillVector.push_back(index); //adds index to list of values in vector
		}
		line = line.substr(endpoint(line) + 1, line.size() - endpoint(line)); //remove range added from range list
	}
}

void getStrings(std::string_view line, std::vector<std::string>& fillVector)
{
	std::size_t(*endpoint)(std::string_view)(&findNextSemi); //initializes endpoint function pointer and sets it to point to find next semi
	bool loopAgain{ true }; //controls whether or not the loop will continue iterating
	while (loopAgain)	//loops while more time available ranges exist (while dividers exist plus once more)
	{
		if (line.find(':') == std::string::npos)//if range divider does not exist, stop looping after this iteration and search for boundary to times per cycle instead of between ranges
		{
			loopAgain = false; //stop iterating (this is the last range)
			endpoint = &findEnd; //set endpoint to comma (since that is the divider between csv columns)
		}

		fillVector.emplace_back(line.substr(0, endpoint(line))); //adds index to list of values in vector
		line = line.substr(endpoint(line) + 1, line.size() - endpoint(line)); //remove range added from range list
	}
}

//adds activity to activities array, returns times per cycle for activity
int addActivity(std::string line, std::vector <Activity>& activities, const int activityID)
{
	std::size_t comma{ line.find(',') }; //find break between activity name and activity times available
	std::string activityName{ line.substr(0,comma) }; //stores activity name
	line = line.substr(comma + 1, line.size() - comma - 1); //removes activty name from line


	std::vector < std::size_t > timesAvailable{};//array storing if activity is available at each time slot


	getValues(line, timesAvailable); //gets times available from line and adds it to times avaliable vector

	int timesPerCycle{ std::stoi(line) }; //rest of line after times avaible is times per cycle

	activities.push_back(Activity(activityName, timesAvailable, timesPerCycle, activityID)); //add activity to activities array
	return timesPerCycle;
}

//add activity category to categories vector
//note: activities is non const since ActivityCategory intializer moves activity to its list of activities
void createActivityCategory(std::vector <ActivityCategory>& categories, std::string_view category, std::vector<Activity>& activities)
{
	std::sort(activities.begin(), activities.end(), [](Activity first, Activity second) //sorts activity list by total times available
		{
			return first.getTotalTimesAvailable() < second.getTotalTimesAvailable();
		});
	categories.push_back(ActivityCategory(category, activities));
}

//takes in a lsit of activity anmes to search for and the list of activity categories and fills a list of pointers to those activities
//note categories cannot be const since their activities are added to the activity list which may later be changed via their pointer
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

//initializes static array scheduleSlots before use
std::array<ScheduleSlot, daysInCycle* periodsInDay> ParticipantGroup::scheduleSlots{};

//takes in list of avilable times for a staff to elad at and adds a pointer to the schedule slot at each of those times to the scheduleSlotsAvailable list
void getScheduleSlots(const std::vector<std::size_t> avail, std::vector<ScheduleSlot*> scheduleSlotsAvailable)
{
	for (std::size_t i{ 0 }; i < avail.size(); ++i)
	{
		scheduleSlotsAvailable.push_back(&ParticipantGroup::scheduleSlots[i]);
	}
}

//takes in a string and a breakpoint and fiils the inputted activity pointers vector with activity pointers to the activites found within the string
void processActivitiesListFromFileToVectorofActivityPointers(std::string_view line, std::vector<Activity*> &activityPointers, std::vector <ActivityCategory>& categories, const std::size_t breakLocation)
{
	std::string raw{ line.substr(0,breakLocation) };  //hold raw list of activities
	std::vector<std::string> names{}; //holds list of activities names
	getStrings(raw, names); //processes raw list and fills list of activities names
	getActivities(names, activityPointers, categories); //gets list of pointers to activities using their names and fills activity pointers vector
}

//rads in staff from file and stores in the staff vector
//note cateogries is non const due to getActivities
void readInStaff(std::ifstream& myReader, std::vector <ActivityCategory>& categories, std::vector <Staff> &staff)
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

		line = line.substr(breakLocation + 1, line.size()); //line removes neutral names and break
		std::vector<Activity*> unpreferred{}; //holds list of pointers to preffered activites
		breakLocation = line.find(','); //location of next breakpoint (at the end of the list of unpreferred activites)
		processActivitiesListFromFileToVectorofActivityPointers(line, unpreferred, categories, breakLocation); //fills unpreferred vector with pointer to activities between teh previous and current breakpoints

		line = line.substr(breakLocation + 1, line.size()); //line removes unpreferred names and break
		std::vector<std::size_t> staffBreaks{}; //stores vector of break times for this staff member
		getValues(line, staffBreaks); //gets staff breaks from line and adds ranges to staffBreaks vector
		std::size_t j{ 0 }; //holds the time of the next break as we search through all schedule slots
		std::vector<std::size_t> availableTimes{}; //holds all the times when the staff can lead (not break times)
		for (std::size_t i{ 0 }; i < periodsInDay * daysInCycle; ++i) //loops throough all possible schedule slots
		{
			if (i != staffBreaks[j]) //if the schedule slot is not in the break list
				availableTimes.push_back(i); //add the schedule slot time to the times available vector
			else if (j < staffBreaks.size() - 1) //if the schedule slot is in the break list and it is not the last break
				++j; //iterate to check the next break schedule slot
		}
		std::vector<ScheduleSlot*> timesAvailable{}; //holds pointers to the schedule slots corresponding to the times the staff can lead at
		getScheduleSlots(availableTimes, timesAvailable); //fills the list of pointers using the indecies of the times that the staff can lead at 

		staff.emplace_back(name, timesAvailable, preferred, neutral, unpreferred); //adds staff member to staff vector

	}
}

//reads in activity and activity category info and stores it in categories vector
int readInActivityCategories(std::vector <ActivityCategory>& categories, std::vector <Staff>& staff)
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
				createActivityCategory(categories, prevCategory, activities); //create previous activity category
				activities.clear(); //reset activities
			}
			prevCategory = category; //ensures category is updated (important for first iteration)
			timesPerCycle += addActivity(line.substr(comma + 1, line.size() - comma - 1), activities, activityID); //adds activity using line excluidng activity category information
			++activityID; //iterates activity id
		}
		createActivityCategory(categories, prevCategory, activities); //creates final activity category
		readInStaff(myReader, categories,staff); //reads in staff
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
	std::vector <Staff> staff{};

	int maxID{};

	try
	{
		maxID = readInActivityCategories(categories, staff);
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
