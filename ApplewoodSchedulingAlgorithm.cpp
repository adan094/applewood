
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

class Activity;
class Staff; //staff class prototype so it can be referred to in Activity
class ActivityCategory; //Activity Category class prototype so it can be referred to in Activity
class ScheduleSlot; //Schedule Slot class prototype so it can be referred to in Activity

//Wrapper which provides generic implementation for Activity, ScheduleSlot and Staff objects
class SpotWrapper
{

public:

	//stores the possible types of a SpotWrapper
	enum class Type
	{
		Activity,
		ScheduleSlot,
		Staff,
	};

	//made int to prevent underflow errors in its children classes
	virtual constexpr std::pair<int, int> getNumberToDiscard() const = 0; //gets number of options to be discarded before filling the spot
	virtual constexpr Type getType() = 0; //object type
	virtual void removeFromThis(SpotWrapper* spot) = 0; //remove a spot (not a ScheduleSlot) from this object
	std::vector<SpotWrapper*> m_availableSpots{}; //stores the available spots to fill this spot
	int m_index{}; //stores the index in Fill Spot's spotsToBeFilled
	int m_timesPerCycle{}; //number of times this spot should occur in the generated schedule
	int m_timesLeftPerCycle{}; //how many more of this spot should occur
	int m_id{}; //the unique id of the spot
	static int id; //holds id of next spot
	bool m_completed{ false }; //stores whether or not this object has been filled

	std::vector <Activity*> m_activities{}; //a list of the activities filled by this spot
	std::vector <ScheduleSlot*> m_slots{}; //a list of the slots filled by this spot
	std::vector <Staff*> m_staff{};  //a list of the staff filled by this spot


	std::vector <ScheduleSlot*> m_timesAvailable{}; //holds the indices of the schedule slots where this spot can occur


	//the spots with the least variation of options of places to go should be filled first, ties should be solved by least available staff to lead, then most spots left to fill
	//spots are valued based upon on soon they should be filled (soonest = lowest)
	friend bool operator> (SpotWrapper& spot1, SpotWrapper& spot2)
	{

		//if a spot is completed, move to the front of the list so it can be removed
		if (spot1.getCompleted())
			return false;
		if (spot2.getCompleted())
			return true;

		//gets the min and max of the pair of number to discard for each spot
		int spot1Min{ std::get<0>(spot1.getNumberToDiscard()) };
		int spot1Max{ std::get<1>(spot1.getNumberToDiscard()) };
		if (spot1Min > spot1Max)
			std::swap(spot1Min, spot1Max);
		int spot2Min{ std::get<0>(spot2.getNumberToDiscard()) };
		int spot2Max{ std::get<1>(spot2.getNumberToDiscard()) };
		if (spot2Min > spot2Max)
			std::swap(spot2Min, spot2Max);



		//sorts by largest minimum options to discard
		if (spot1Min > spot2Min)
			return true;
		else if (spot1Min < spot2Min)
			return false;

		//breaks ties by largest largest options to discard
		if (spot1Max > spot2Max)
			return true;
		else if (spot1Max < spot2Max)
			return false;

		//break remaining ties by least spots left to fill
		else if (spot1.m_timesLeftPerCycle < spot2.m_timesLeftPerCycle)
			return true;
		else
			return false;
	}

	friend bool operator< (SpotWrapper& spot1, SpotWrapper& spot2)
	{

		//if a spot is completed, move to the front of the list so it can be removed
		if (spot1.getCompleted())
			return true;
		if (spot2.getCompleted())
			return false;

		//gets the min and max of the pair of number to discard for each spot
		int spot1Min{ std::get<0>(spot1.getNumberToDiscard()) };
		int spot1Max{ std::get<1>(spot1.getNumberToDiscard()) };
		if (spot1Min > spot1Max)
			std::swap(spot1Min, spot1Max);
		int spot2Min{ std::get<0>(spot2.getNumberToDiscard()) };
		int spot2Max{ std::get<1>(spot2.getNumberToDiscard()) };
		if (spot2Min > spot2Max)
			std::swap(spot2Min, spot2Max);



		//sorts by largest minimum options to discard
		if (spot1Min < spot2Min)
			return true;
		else if (spot1Min > spot2Min)
			return false;

		//breaks ties by largest largest options to discard
		if (spot1Max < spot2Max)
			return true;
		else if (spot1Max > spot2Max)
			return false;

		//break remaining ties by least spots left to fill
		else if (spot1.m_timesLeftPerCycle > spot2.m_timesLeftPerCycle)
			return true;
		else
			return false;
	}

	//gets spots that can fill this spot
	std::vector<SpotWrapper*>& getAvailableSpots()
	{
		return m_availableSpots;
	}

	//gets index of this spot
	constexpr int getIndex() const
	{
		return m_index;
	}

	//sets the index of this spot
	void setIndex(int index)
	{
		m_index = index;
	}

	//gets the id of this spot
	constexpr int getID() const
	{
		return m_id;
	}

	//gets timesAvailable array
	std::vector <ScheduleSlot*>& getTimesAvailable()
	{
		return m_timesAvailable;
	}

	//removes a given spot from a given array in this spot
	template <typename T>
	bool removeSpot(SpotWrapper* spot, std::vector<T*>& array)
	{
		for (std::size_t index{ 0 }; index < array.size(); ++index)
		{
			if (array[index] == spot)
			{
				array.erase(array.begin(),array.end()+index);
				return true;
			}
		}
		return false;
	}

	//adds two spots to this spot
	void add(SpotWrapper* spot1, SpotWrapper* spot2)
	{
		add(spot1);
		add(spot2);
	}

	//gets the completed status of this spot
	constexpr bool getCompleted() const
	{
		return m_completed;
	}

	//gets how many times this spot occurs
	constexpr int getTimesPerCycle() const
	{
		return m_timesPerCycle;
	}

 //sets number of times this spot will occur
 void setTimesPerCycle (const int timesPerCycle)
 {
  m_timesPerCycle=timesPerCycle;
 }

 //sets how many more times this spot will occur
 void setTimesLeftPerCycle (const int timesLeftPerCycle)
 {
  m_timesLeftPerCycle=timesLeftPerCycle;
 }

 //increases the number of times this spot will occur by 1
 void incrementTimesPerCycle ()
 {
  ++m_timesPerCycle;
 }

 //increases how many more times this spot will occur by 1
 void incrementTimesLeftPerCycle ()
 {
  ++m_timesLeftPerCycle;
 }

	void remove(SpotWrapper* spot);
	void add(SpotWrapper* spot);

};




//Each room has a name and group level capacity
struct Room
{
 std::string name;
 int capacity;
};

//converts char to Level
Level getLevel(const char c) const
{
 switch(c)
 {
  case ('A'):
   return Level::A;
  case('B'):
   return Level::B;
  case('C'):
   return Level::C;
  throw "Invlaid char to level conversion";
 }
}

enum class Level
{
 A,
 B,
 C,
 maxLevel, //used for finding number of levels
};

// Represents each schedule time period
class ScheduleSlot : public SpotWrapper
{
 int m_numberOfParticipants{0}; //the number of participants participating in this scheduleSlot

	const int m_time{ 0 }; //the time which this schedule slot takes place at
	std::vector <Activity*> m_possibleActivities{}; //A list of possible activities to occur in this slot
	std::vector <Staff*> m_possibleStaff{}; //A list of possible staff to occur in this slot
 Room *m_room{nullptr}; //a pointer to the room this slot occurs in
 Level m_level{}; //the group level of this scheduleSlot
 std::vector<ScheduleSlot*> m_slotsAtSameTime{}; //the slots that occur at the same time as this slot

public:

	ScheduleSlot() = default;

 

	//intializes schedule slot using the time the slot occurs at and its level
	ScheduleSlot(const int time, const Level level)
		:m_time{ time },
  m_level{level}
	{
		m_timesPerCycle = 1;
		m_timesLeftPerCycle = 1;
		m_id = id; //assings object's unique id as next id to add
		++id; //iterates Spotwrapper ID to ensure each object has a unique ID
	}

	//adds a staff to the availableToLead array
	constexpr void addAvailableToLead(Staff* staff)
	{
		m_possibleStaff.push_back(staff);
	}


	//Discarded options in schedule slot are possible activities to fill the slot -1
	constexpr std::pair<int, int> getNumberToDiscard() const
	{
		return { m_possibleActivities.size() - 1,static_cast<int>(m_possibleStaff.size()) - 1 };
	}

	//Returns the type (for parent container type checking)
	constexpr Type getType()
	{
		return Type::ScheduleSlot;
	}

	//returns the time which this schedule slot occurs at
	constexpr int getTime() const
	{
		return m_time;
	}

	//adds a given staff member to the possible staff to fill this slot
	void addPossibleStaff(Staff* staff)
	{
		m_possibleStaff.push_back(staff);
	}

	//adds a given activity to the possible activities to fill this slot
	void addPossibleActivities(Activity* activity)
	{
		m_possibleActivities.push_back(activity);
	}

	//removes a spot from their respective possible list depending on their type
	void removeFromThis(SpotWrapper* spot)
	{
		if (spot->getType() == Type::Activity)
			removeSpot(spot, m_possibleActivities);
		else
			removeSpot(spot, m_possibleStaff);
	}

//returns the schedule slots occuring at the same time as this slot
 std::vector<ScheduleSlot*>& getSlotsAtSameTime()
 {
  return m_slotsAtSameTime;
 }

 void addSlotAtSameTime(ScheduleSlot *slot)
 {
  m_slotsAtSameTime.push_back(slot);
 }

//adds a participant to this schedule slot
 void addParticipant()
 {
  ++m_numberOfParticipants;
 }

 //gets the number if participants, participating in this schedule slot
 constexpr int getNumberOfParticipants() const
 {
  return m_numberOfParticipants
 }

 //gets possibleActivities array
	std::vector <Activity*>& getActivitiesAvailable()
	{
		return m_possibleActivities;
	}

 //gets possibleActivities array
	std::vector <Staff*>& getStaffAvailable()
	{
		return m_possibleStaff;
	}


};

//Represents each activity
class Activity :public SpotWrapper
{
	ActivityCategory* m_activityCategory{ nullptr }; //holds a pointer to the activity category which this activity belongs to

	std::string m_activityName{}; //the display name of the activity

	std::vector <Staff*> m_preferredStaff{}; //a list of the staff who prefer to lead this spot
	std::vector <Staff*> m_neutralStaff{}; //a list of the staff who are neutral towards leading this spot
	std::vector <Staff*> m_unpreferredStaff{}; //a list of the staff who prefer not to lead this spot

 std::vector<Room*> potentialRooms{}; //a list of pointers to all rooms this activity can occur in

	//adds list of possible activities to this slot and adds this slot to the timeavailable of each of those activities
	void setTimesAvailable(std::vector<ScheduleSlot*> &possibleSlots)
	{
		m_timesAvailable = std::move(possibleSlots);
		for (auto slot : m_timesAvailable)
		{
			m_availableSpots.push_back(slot);
			slot->m_availableSpots.push_back(this);
			slot->addPossibleActivities(this);
		}
	}

public:

	Activity() = default; //a default constructor with no arguments

	//creates Activity using its display name, how many times and when it should happen.
	Activity(const std::string_view activityName, const int timesPerCycle, std::vector<ScheduleSlot*> &possibleTimes)
		: m_activityName{ activityName }
	{
		setTimesAvailable(possibleTimes);
		m_timesPerCycle = timesPerCycle;
		m_timesLeftPerCycle = timesPerCycle;
		m_id = id; //assings object's unique id as next id to add
		++id; //iterates Spotwrapper ID to ensure each object has a unique ID
	}



	//gets this activity's name
	constexpr std::string_view getName() const
	{
		return m_activityName;
	}

	//sets the activity category of this activity
	void setActivityCategory(ActivityCategory* activityCategory)
	{
		m_activityCategory = activityCategory;
	}

	//Discarded slots are spots - number of spots to fill, discarded staff can be as high as staff since they can lead the activity many times
	constexpr std::pair<int, int> getNumberToDiscard() const
	{
		return { m_timesAvailable.size() - m_timesLeftPerCycle, static_cast<int>(m_preferredStaff.size() + m_neutralStaff.size() + m_unpreferredStaff.size()) - 1};
	}


	//Returns the type (for parent container type checking)
	constexpr Type getType()
	{
		return Type::Activity;
	}

	//adds a preferred staff member to this activity
	void addPreferredStaff(Staff* staff)
	{
		m_preferredStaff.push_back(staff);
	}

	//adds a neutral staff member to this activity
	void addNeutralStaff(Staff* staff)
	{
		m_neutralStaff.push_back(staff);
	}

	//adds an unpreferred staff member to this activity
	void addUnpreferredStaff(Staff* staff)
	{
		m_unpreferredStaff.push_back(staff);
	}

	//Must be a staff member since activities only reference them and slots and slots are checked by remove prior to calling this function
	//removes a staff member from this spot
	void removeFromThis(SpotWrapper *spot)
	{
		if (!removeSpot(spot, m_preferredStaff))
		{
			if (!removeSpot(spot, m_neutralStaff))
				removeSpot(spot, m_unpreferredStaff);
		}
	}

 //gets m_preferredStaff array
	std::vector <Staff*>& getPreferredStaff()
	{
		return m_preferredStaff;
	}

 //gets m_neutralStaff array
	std::vector <Staff*>& getNeutralStaff()
	{
		return m_neutralStaff;
	}

 //gets m_unpreferredStaff array
	std::vector <Staff*>& getUnpreferredStaff()
	{
		return m_unpreferredStaff;
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
				if (a1.getTimesAvailable().size() < a2.getTimesAvailable().size())
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


//stores staff members
class Staff : public SpotWrapper
{
	std::string m_name{}; //staff name

	std::vector<Activity*> m_preferredActivities{}; //holds the list of activities that this staff would prefer to lead
	std::vector<Activity*> m_neutralActivities{}; //holds the list of activities that this staff feels neutral towards leading
	std::vector<Activity*> m_unpreferredActivities{}; //holds the lsit of activities that this staff would not prefer to lead

	//adds list of preferred activities to this staff member and adds this staff to the preferred staff of each of those activities
	void setPreferredActivities(std::vector<Activity*> preferredActivities)
	{
		m_preferredActivities = std::move(preferredActivities);
		for (auto activity : m_activities)
		{
			m_availableSpots.push_back(activity);
			activity->m_availableSpots.push_back(this);
			activity->addPreferredStaff(this);
		}
	}

	//adds list of neutral activities to this staff member and adds this staff to the neutral staff of each of those activities
	void setNeutralActivities(std::vector<Activity*> neutralActivities)
	{
		m_neutralActivities = std::move(neutralActivities);
		for (auto activity : m_activities)
		{
			m_availableSpots.push_back(activity);
			activity->m_availableSpots.push_back(this);
			activity->addNeutralStaff(this);
		}
	}

	//adds list of unpreferred activities to this staff member and adds this staff to the unpreferred staff of each of those activities
	void setUnpreferredActivities(std::vector<Activity*> unpreferredActivities)
	{
		m_unpreferredActivities = std::move(unpreferredActivities);
		for (auto activity : m_activities)
		{
			m_availableSpots.push_back(activity);
			activity->m_availableSpots.push_back(this);
			activity->addUnpreferredStaff(this);
		}
	}

	//adds list of available schedule slots to this staff member and adds this staff to the avaialble staff of each of those schedule slots
	void setTimesAvailable(std::vector<ScheduleSlot*> timesAvailable)
	{
		m_timesAvailable = std::move(timesAvailable);
		for (auto slot : m_timesAvailable)
		{
			m_availableSpots.push_back(slot);
			slot->m_availableSpots.push_back(this);
			slot->addPossibleStaff(this);
		}
	}
public:

	//Staff constructor, initializes name, available activity lists, slots available list, times per cycle and id
	Staff(const std::string_view name, const int timesPerCycle, std::vector <Activity*> &preferredActivities, std::vector <Activity*>& neutralActivities, std::vector <Activity*>& unpreferredActivities, std::vector<ScheduleSlot*> &slots)
		: m_name{ name }
	{
		setPreferredActivities(preferredActivities);
		setNeutralActivities(neutralActivities);
		setUnpreferredActivities(unpreferredActivities);
		setTimesAvailable(slots);
		m_timesPerCycle = timesPerCycle;
		m_timesLeftPerCycle = timesPerCycle;
		m_id = id; //assings object's unique id as next id to add
		++id; //iterates Spotwrapper ID to ensure each object has a unique ID
	}

	//gets number of options to be discarded before filling the spot
	constexpr std::pair<int, int> getNumberToDiscard() const
	{
		return { m_preferredActivities.size() + m_neutralActivities.size() + m_unpreferredActivities.size() - m_timesLeftPerCycle,m_timesAvailable.size() - m_timesLeftPerCycle};
	}

	//gets object type
	constexpr Type getType()
	{
		return Type::Staff;
	}

	//Must be an activity since spots only reference them and slots and slots are checked by remove prior to calling this function
	//removes an activity from this spot
	void removeFromThis(SpotWrapper* spot)
	{
		if (!removeSpot(spot, m_preferredActivities))
		{
			if (!removeSpot(spot, m_neutralActivities))
				removeSpot(spot, m_unpreferredActivities);
		}
	}

 //gets m_preferredActivities array
	std::vector <Activity*>& getPreferredActivities()
	{
		return m_preferredActivities;
	}

 //gets m_neutralActivities array
	std::vector <Activity*>& getNeutralActivities()
	{
		return m_neutralActivities;
	}

 //gets m_unpreferredActivities array
	std::vector <Activity*>& getUnpreferredActivities()
	{
		return m_unpreferredActivities;
	}


};

int SpotWrapper::id{ 0 }; //initiailize the starting id of the SpotWrapper class




//removes this spot from the lists of a given spot
void SpotWrapper::remove(SpotWrapper* spot)
{
	removeSpot(spot, m_availableSpots);

	if (spot->getType() == Type::ScheduleSlot)
	{
		removeSpot(static_cast<ScheduleSlot*>(spot), m_timesAvailable);
	}
	else
	{
		removeFromThis(spot);
	}
}



//adds a given spot to this spot and removes this spot from other spots if it has been filled
void SpotWrapper::add(SpotWrapper* spot)
{
	if (spot->getType() == Type::Activity)
	{
		m_activities.push_back(static_cast<Activity*>(spot));
	}
	else if (spot->getType() == Type::ScheduleSlot)
	{
		m_slots.push_back(static_cast<ScheduleSlot*>(spot));
  //removes staff from list of availableStaff at the schedule slors at the same as the one they are being added to
  for(ScheduleSlot* slot: m_slots[m_slots.size()-1).getSlotsAtSameTime())
  {
			  slot->remove(this);
  }
 }
			
	else
	{
		m_staff.push_back(static_cast<Staff*>(spot));
	}

	--m_timesLeftPerCycle; //decreases the times left to add to this spot

	//if this spot is full, remove it from the possible list of all its possible spot
	if (m_timesLeftPerCycle == 0)
	{
		m_completed = true;
		
		for(SpotWrapper* availableSpot: m_availableSpots)
			availableSpot->remove(this);
	}
}

class FillSpot
{

	//prevents typos from causes bugs
	enum sortDirection //possible sort directions of update sort
	{
		either,
		decreased
	};

	std::vector < SpotWrapper*> m_spotsToBeFilled; //Holds all the activities and schedule slots to be filled
	std::vector <Activity> m_activities; //Holds activities and ensures their existence for the lifetime of the class
	std::vector <ScheduleSlot> m_scheduleSlots; //Holds schedule slots and ensures their existence for the lifetime of the class
	std::vector <Staff> m_staff; //Holds staff and ensures their existence for the lifetime of the class

	//returns whether the spot at the given index of the spots to be filled array is available within a given spot
	bool foundIndex(SpotWrapper* spot, int index)
	{
		if (std::find(spot->getAvailableSpots().begin(), spot->getAvailableSpots().end(), m_spotsToBeFilled[index]) == spot->getAvailableSpots().end())
			return false;
		return true;
	}

std::size_t m_nextIndex{ 0 };

	//gets the spot with the lowest index belonging to a given spot
	SpotWrapper* getNext(SpotWrapper* spot)
	{
		if(m_nextIndex!=0)
   ++m_nextIndex;
		try
		{
			while (foundIndex(spot, index))
				++m_nextIndex;
		}
		catch (...)
		{
			throw "Nothing can fill this spot";
		}
		return m_spotsToBeFilled[index];
	}

	//gets the spot with the lowest index belonging to the union of two given spots
	SpotWrapper* getFirst(SpotWrapper* spot1, SpotWrapper* spot2)
	{
		std::size_t index{ 0 };
		try
		{
			while (!foundIndex(spot1, index) || !foundIndex(spot2, index))
				++index;
		}
		catch (...)
		{
			return null;
		}
		return m_spotsToBeFilled[index];
	}

	//updates the spots to be filled list and its members indices
	void updateSpotsToBeFilled()
	{
		std::sort(m_spotsToBeFilled.begin(), m_spotsToBeFilled.end()); //sorts by how soon the slot should be filled

		std::size_t endOfCompleted{ 0 };

		//removes already filled spots from list
		while (m_spotsToBeFilled[endOfCompleted]->getCompleted())
			++endOfCompleted;
		m_spotsToBeFilled.erase(m_spotsToBeFilled.begin(), m_spotsToBeFilled.begin() + endOfCompleted);

		//updates the index of each spot to be filled
		for (std::size_t index{ endOfCompleted }; index < m_spotsToBeFilled.size(); ++index)
			m_spotsToBeFilled[index]->setIndex(index);
	}


public:

	//initializes the fillspot list of schedule slots, activities and staff. Sorts by how soon the slot should be filled and assigns the respective index in that list to each spot
	FillSpot(std::vector < Activity>& activities, std::vector < ScheduleSlot>& scheduleSlots, std::vector < Staff>& staff)
		:m_activities{ std::move(activities) },//uses std::move for efficiency
		m_scheduleSlots{ std::move(scheduleSlots) }, //uses std::move for efficiency
		m_staff{ std::move(staff) }
	{

		for (Activity& activity : m_activities) //adds pointers to all activities to spotsToBeFilled
		{
			m_spotsToBeFilled.push_back(&activity);
		}

		for (ScheduleSlot& scheduleSlot : m_scheduleSlots) //adds pointers to all scheduleSlots to spotsToBeFilled
		{
			m_spotsToBeFilled.push_back(&scheduleSlot);
		}

		for (Staff& staff : m_staff) //adds pointers to all scheduleSlots to spotsToBeFilled
		{
			m_spotsToBeFilled.push_back(&staff);
		}

		updateSpotsToBeFilled(); //updates the spots to be filled list and its members indices

	}

	//fills the next spot in the lsit and updates all spots as needed
	SpotWrapper* fillNextSpot()
	{

		SpotWrapper* item1{ m_spotsToBeFilled[0] }; //gets the first spot as the first spot in the spots to be filled list
  
  SpotWrapper* item2{null};

  
  
  
  SpotWrapper* item3{null}; 

 //gets the third spot as the first spot from the union of the first and second spot
  while(item3 == null) //while there is no common item bwtween item 1 and 2
  {
   
   try
   {
		  item2 = getFirst(item1) ; //gets the second spot as the first spot's next spot
		 }
   catch("Nothing can fill this spot")
   {
    return item1; //if nothing can fill the spots return the first spot
   }
   //gets the third spot as the first spot from the union of the first and second spot
   item3 = getFirst(item1,item2);
  }

		item1->add(item2, item3); //adds the second and third spot to the first one and removes the first spot from the possible lists of the second and third spots if necessary
		item2->add(item1, item3); //adds the first and third spot to the second one and removes the second spot from the possible lists of the first and third spots if necessary
		item3->add(item1, item2); //adds the first and second spot to the third one and removes the third spot from the possible lists of the first and second spots if necessary

		updateSpotsToBeFilled(); //updates the spots to be filled list and its members indices

	 return null;
 }

//adds 1 times to fill and times left to fill to slot with given id
 void addToSlot(const int id)
 {
  //finds spot with given id
  SpotWrapper *found {std::find(m_spotsToBeFilled.begin(), m_spotsToBeFilled.end(), [&id](SpotWrapper *spot)
  {
   if(spot->getID()==id)
    return true;
   return false;
  })};
  //increments times to fill and times left to fill
  found->incrementTimesToFill();
  found->incrementTimesLeftToFill();
 }
};

















class ParticipantGroup
{
	int m_participants{};
	std::vector <ScheduleSlot> m_ScheduleSlots{};
 std::vector <Activity> m_activities{};
 std::vector <Staff> m_staff{};
	int m_totalTimeSlots{};
 int m_startOfListID {};
 int m_endOfListID {};
 int m_unfilledSlots {};
 Spotwrapper* startOfListPointer{};

//Removes all out of scope schedule slots from this spot's possible list and reassigns pointers to this group's copy
 void prunePossibleSlots(SpotWrapper &spot)
 {
   std::size_t index{0}; //keeps track of spot's possible slots we are iterating through
   std::vector <ScheduleSlot*>* possibleSlots{&spot.getTimesAvailable()}; //a modifiable list of this spot's possible slots
   while(index<possibleSlots->size()) //while there are more spots to check
   {
    int indexID {(*possibleSlots)[index]->getID()};

    //if the id of the slot is outside the slots used by this participant group, remove it from the list
    if(indexID<m_startOfListID||indexID>m_endOfListID)
     possibleSlot->erase(possibleSlots->begin+index);

    //otherwise replace it with a reference to the copy
    else
    {
     possibleSlots[index]=&m_possibleSlots[possibleSlots[index]->getID()-m_startOfListID];
     ++index;
    }
   }
 }

 //reseats all pointers in given list to their local copy
 void reseatPointers(std::vector<SpotWrapper*> *spots, std::vector<SpotWrapper*> newSpots)
 {
  for(SpotWrapper *spot: *spots)
    spot = &newSpots[spot->getID()-newSpots[0].getID()];
 }

 //preforms prune actions on activity's possible pointers
 void pruneActivities(std::vector <Activity*> &activitiesToFill)
 {
  for(Activity &activity: m_activities) //for each activity
  {
   //resets times per cycle and times left per cycle
   activity->setTimesPerCycle{0};
   activity->setTimesLeftPerCycle{0};

   prunePossibleSlots(activity);
   //reseat possible staff vectors
  
   std::vector <Staff*>* preferredStaff{&activity.getPreferredStaff()}; //a modifiable list of this activity's preferred staff
   //reassigns all preferred staff for this activitiy and reassigns them to copies in this object
   reseatPointers(preferredStaff, m_staff);

   std::vector <Staff*>* neutralStaff{&activity.getNeutralStaff()}; //a modifiable list of this activity's neutral staff
   //reassigns all neutral staff for this activitiy and reassigns them to copies in this object
   reseatPointers(neutralStaff, m_staff);

   std::vector <Staff*>* unpreferredStaff{&activity.getUnpreferredStaff()}; //a modifiable list of this activity's unpreferred staff
   //reassigns all unpreferred staff for this activitiy and reassigns them to copies in this object
   reseatPointers(unpreferredStaff, m_staff);
  }

  //increment each copy of activity corresponding with list to fill to get proper spots to fill for each activity
  for(std::size_t index{0}; index<m_scheduleSlots.size(); ++index)
  {
   m_activities[activitiesToFill[index].getID()-m_activities[0].getID()].incrementTimesPerCycle();
   m_activities[activitiesToFill[index].getID()-m_activities[0].getID()].incrementTimesLeftPerCycle();
  }
 }

 //preforms prune actions on staff's possible pointers
 void pruneStaff(std::vector <Staff*> &staffToFill)
 {
  for(Staff &staff: m_staff) //for each staff
  {
  //resets times per cycle and times left per cycle
   staff->setTimesPerCycle{0};
   staff->setTimesLeftPerCycle{0};

   prunePossibleSlots( staff );

   //reseat possible activity vectors
  
   std::vector <Activity*>* preferredActivities{&staff.getPreferredActivities()}; //a modifiable list of this activity's preferred staff
   //reassigns all preferred activities for this staff member and reassigns them to copies in this object
   reseatPointers(preferredActivities, m_activities);

   std::vector <Activity*>* neutralActivities{&staff.getNeutralActivities()}; //a modifiable list of this activity's neutral staff
   //reassigns all neutral activities for this staff member and reassigns them to copies in this object
   reseatPointers(neutralActivities, m_activities);

   std::vector <Activity*>* unpreferredActivities{&staff.getUnpreferredActivities()}; //a modifiable list of this activity's unpreferred staff
   //reassigns all unpreferred activities for this staff member and reassigns them to copies in this object
   reseatPointers(unpreferredActivities, m_activities);
  }
  //increment each copy of staff corresponding with list to fill to get proper spots to fill for each activity
  for(std::size_t index{0}; index<m_scheduleSlots.size(); ++index)
  {
   m_staff[staffToFill[index].getID()-m_staff[0].getID()].incrementTimesPerCycle();
   m_staff[staffToFill[index].getID()-m_staff[0].getID()].incrementTimesLeftPerCycle();
  }
 }

 //reseats pointers to copy lists for activity and staff lists in each schedule slot copy
 void pruneScheduleSlots
 {
  //for each slot
  for(ScheduleSlot &slot: m_scheduleSlots)
  {
   std::vector <Activity*>* possibleActivities{&slot.getActivitiesAvailable()}; //a modifiable list of this slot's possible activities
   std::vector <Staff*>* possibleStaff{&slot.getStaffAvailable()}; //a modifiable list of this slot's possible staff

   //reassigns all possible activities in this slot and reassigns them to copies in this object
   reseatPointers(possibleActivities, m_activities);
   //reassigns all possible staff in this slot and reassigns them to copies in this object
   reseatPointers(possibleStaff, m_staff);
  }
 }

//fills participant group
 void fill(std::vector <Activity*> &activitiesToFill, std::vector <Staff*> &staffToFill)
 {
  int numberOfScheduleSlots {m_scheduleSlots.size()};
  FillSpot filler(m_activities, m_scheduleSlots, m_staff);
  //fill each slot in list

  SpotWrapper* unfillable {null};
  int swapIndex {0};
  for(std::size_t index{0}; index<numberOfScheduleSlots; ++index))
  {
   unfillable={filler.fillNextSpot()};
   //if the spot to be added cannot be added
   if(unfillable!=null)
   {
    //swap it out and try again
    unfillable->setTimesPerCycle(unfillable->getTimesPerCycle()-1);
    unfillable->setTimesLeftPerCycle(unfillable->getTimesPerCycle()-1);

    if(unfillable->getType()==Type::Activity)
    {
     filler.addToTime(activitiesToFill[m_scheduleSlots.size()+index]->getID());
     std::swap(activitiesToFill[m_scheduleSlots.size()+index], activitiesToFill[unfillable->getID()-m_activities[0].getID()]);
    }
    else if(unfillable->getType()==Type::Staff)
    {
     filler.addToTime(staffToFill[m_scheduleSlots.size()+index]->getID());
     std::swap(staffToFill[m_scheduleSlots.size()+index], staffToFill[unfillable->getID()-m_staff[0].getID()]);
    }
    else
    {
     Spotwrapper * found {std::find(startOfListPointer, startOfListPointer+endOfListID-startOfListID, [unfillable] (Spotwrapper* spot)
     {
      if(spot->getID()==unfillable->getID())
       return true;
      return false;
     })};
     std::swap(startOfListPointer+endOfListID-startOfListID-m_unfilledSlots,found);
     ++m_unfilledSlots;
    }

    --index;
    ++swapIndex;
   }
  }
 }



public:

	static std::array<ScheduleSlot, daysInCycle* periodsInDay> scheduleSlots;

	ParticipantGroup() = default;

 //use given pointers and lists to copy list of Schedule Slots, activities and staff and initialize member variables
 ParticipantGroup(const ScheduleSlot* startOfList, const ScheduleSlot* endOfList, const std::vector<Activity> &activities, const std::vector<Staff> &staff, const int numberOfFilledSlots, std::vector<Activity> &activitiesToFill, std::vector<Staff> &staffToFill)
 :m_startOfListPointer {startOfList},
  m_startOfListID {startOfList->getID()},
  m_endOfListID {endOfList->getID()},
  m_scheduleslots {std::copy(startOfList, endOfList)}, //gets copy so that we can fill spots using only slots in this group
  m_participants {m_scheduleSlots[0]->getParticipants()},
  m_totalTimeSlots {m_scheduleSlots.size()},
  m_activities {activities},
  m_staff {staff}
 {
  pruneActivities(activitiesToFill);
  pruneStaff(staffToFill);
  pruneScheduleSlots();
  fill(activitiesToFill, staffToFill);
 }

 //gets total time slots
	constexpr int getTotalTimeSlots() const
	{
		return m_totalTimeSlots;
	}



	//prints out cyclical schedule
	void printCycleSchedule()
	{
		for (std::size_t j{ 0 }; j < daysInCycle; ++j) //for each day in cycle
		{
			for (std::size_t i{ 0 }; i < periodsInDay; ++i) //for each period in day, print the schedule slot
				std::cout << i + 1 << ". " << scheduleSlots[j * 10 + i].getActivity()->getName() << "\n";
			std::cout << "---------------------------\n"; //divider between days
		}
	}

 constexpr int getUnfilledSlots() const
 {
  return m_unfilledSlots;
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


void getScheduleSlots(std::string& line, std::vector<ScheduleSlot *>& fillVector, std::vector <ScheduleSlot> &scheduleSlots, const bool oneRange, const int offset)
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
   if(oneRange)
			 fillVector.push_back(&scheduleSlots[index+offset]); //adds a pointer to the scheduleSlots at the given time to the list of times it can occur at
		 else
   {
    for(std::size_t j {index}; j<Level::maxLevel*daysInCycle*periodsInDay; j+=daysInCycle*PeriodsInDay)
     fillVector.push_back(&scheduleSlots[j]);
   }
  }
		line = line.substr(endpoint(line) + 1, line.size() - endpoint(line)); //remove range added from range list
	}
}

void getScheduleSlots(std::string& line, std::vector<ScheduleSlot *>& fillVector, std::vector <ScheduleSlot> &scheduleSlots)
{
 getScheduleSlots(line,fillVector,scheduleSlots, false, 0);
}

void getScheduleSlots(std::string& line, std::vector<ScheduleSlot *>& fillVector, std::vector <ScheduleSlot> &scheduleSlots, const int offset)
{
 getScheduleSlots(line, fillVector, scheduleSlots, true, offset);
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

//add activity category to categories vector
//note: activities is non const since ActivityCategory intializer moves activity to its list of activities
void createActivityCategory(std::vector <ActivityCategory>& categories, std::string_view category, std::vector<Activity>& activities)
{
	std::sort(activities.begin(), activities.end(), [](Activity first, Activity second) //sorts activity list by total times available
		{
			return first.getTimesAvailable().size() < second.getTimesAvailable().size();
		});
	categories.push_back(ActivityCategory(category, activities));
}

//takes in a list of activity names to search for and fills a list of pointers to those activities
//note activities cannot be const since they are added to the activity list which may later be changed via their pointer
void getActivities(const std::vector<std::string>& activityNames, std::vector<Activity*>& activityPointers, std::vector <Activity>& activities)
{
	for (std::size_t i{ 0 }; i < activityNames.size(); ++i) //loops through all activities we are searching for
	{
		bool loopAgain{ true }; //control variable that allows the process to skip to searching for next activity once the activity has been found
			for (std::size_t k{ 0 }; k < activities->size() && loopAgain; ++k) //loops through activities while there are more to search and the activity has not yet been found
			{

				if (activities[k].getName() == activityNames[i]) //if the activity is the activity we are searching for (it has the same name as the next activity in the list of activities we are searching for)
				{
					activityPointers.push_back(&(activites[k])); //add a pointer to the activity to the activities vector
					loopAgain = false; //marks activity as found, allowing process to skip to next activity
				}
			}
		}
	}
}

//initializes static array scheduleSlots before use
std::array<ScheduleSlot, daysInCycle* periodsInDay> ParticipantGroup::scheduleSlots;

//takes in list of avilable times for a staff to elad at and adds a pointer to the schedule slot at each of those times to the scheduleSlotsAvailable list
void getScheduleSlots(const std::vector<std::size_t> avail, std::vector<ScheduleSlot*> scheduleSlotsAvailable)
{
	for (std::size_t i{ 0 }; i < avail.size()*Level::maxLevel; ++i)
	{
		scheduleSlotsAvailable.push_back(&ParticipantGroup::scheduleSlots[i%(periodsInDay*daysInCycle)]);
	}
 //adds slots at the same time to each slot's list
 for(std::size_t i{0}; i < avail.size()*Level::maxLevel; ++i)
 {
  for(std::size_t j{i%avail.size()}; j < Level::maxLevel; j+=avail.size())
  {
   if(i!=j)
    slot[i].addSlotAtSameTime(&slot[j]);
  }
 }
}

//takes in a string and a breakpoint and fiils the inputted activity pointers vector with activity pointers to the activites found within the string
void processActivitiesListFromFileToVectorofActivityPointers(std::string_view line, std::vector<Activity*>& activityPointers, std::vector <Activity>& activities, const std::size_t breakLocation)
{
	std::string raw{ line.substr(0,breakLocation) };  //hold raw list of activities
	std::vector<std::string> names{}; //holds list of activities names
	getStrings(raw, names); //processes raw list and fills list of activities names
	getActivities(names, activityPointers, activities); //gets list of pointers to activities using their names and fills activity pointers vector
}

//reads in staff from file and stores in the staff vector
//note activities is non const due to getActivities
void readInStaff(std::ifstream& myReader, std::vector <Activity>& activities, std::vector <Staff>& staff)
{
	std::string line{};//holds line data
	while (true) //iterates for each staff in the file
	{

  std::getline(myReader, line);
		std::size_t breakLocation{ line.find(',') };//location of break between staff name and preferred lead activities
		std::string name{ line.substr(0,breakLocation) }; //holds staff name

  if(name=="Partcipants") //Once participants is read in, starts to read in participants
   break;


		line = line.substr(breakLocation + 1, line.size()); //line removes staff name and break
		std::vector<Activity*> preferred{}; //holds list of pointers to preffered activites
		breakLocation = line.find(','); //location of next breakpoint (at the end of the list of preffered activites)
		processActivitiesListFromFileToVectorofActivityPointers(line, preferred, activities, breakLocation); //fills preferred vector with pointer to activities between the previous and current breakpoints

		line = line.substr(breakLocation + 1, line.size()); //line removes list of preferred names and break
		std::vector<Activity*> neutral{}; //holds list of pointers to preffered activites
		breakLocation = line.find(','); //location of next breakpoint (at the end of the list of neutral activites)
		processActivitiesListFromFileToVectorofActivityPointers(line, neutral, activities, breakLocation); //fills neutral vector with pointer to activities between the previous and current breakpoints

		line = line.substr(breakLocation + 1, line.size()); //line removes neutral names and break
		std::vector<Activity*> unpreferred{}; //holds list of pointers to preffered activites
		breakLocation = line.find(','); //location of next breakpoint (at the end of the list of unpreferred activites)
		processActivitiesListFromFileToVectorofActivityPointers(line, unpreferred, activities, breakLocation); //fills unpreferred vector with pointer to activities between the previous and current breakpoints

		line = line.substr(breakLocation + 1, line.size()); //line removes unpreferred names and break
		std::vector<std::size_t> staffBreaks{}; //stores vector of break times for this staff member
		getValues(line, staffBreaks); //gets staff breaks from line and adds ranges to staffBreaks vector
		std::size_t j{ 0 }; //holds the time of the next break as we search through all schedule slots
		std::vector<ScheduleSlot*> availableTimes{}; //holds all the schedule slots where the staff can lead (not break times)
		for (std::size_t i{ 0 }; i < periodsInDay * daysInCycle; ++i) //loops throough all possible schedule slots
		{
			if (i != staffBreaks[j]) //if the schedule slot is not in the break list
				availableTimes.push_back(&scheduleSlots[i]); //add a pointer to the schedule slot to the times available vector
			else if (j < staffBreaks.size() - 1) //if the schedule slot is in the break list and it is not the last break
				++j; //iterate to check the next break schedule slot
		}
		std::vector<ScheduleSlot*> timesAvailable{}; //holds pointers to the schedule slots corresponding to the times the staff can lead at
		getScheduleSlots(availableTimes, timesAvailable); //fills the list of pointers using the indecies of the times that the staff can lead at 

		staff.emplace_back(name, 10, preferred, neutral, unpreferred, timesAvailable); //adds staff member to staff vector

	}
}

//reads in activitiy info, creates activity objects and stores them in activities vector
void readInActivities(std::ifstream& myReader, std::vector <Activity>& activities, std::vector <ScheduleSlot>& scheduleSlots)
{

		std::string line{};//holds line data
		std::getline(myReader, line); //skips first line (column headers)

		while (true) //loops until broken (when staff starts to be read in), reasds one activity at a time ine
		{
			std::getline(myReader, line); //gets line
			std::size_t comma{ line.find(',') };//location of break between category name and activity name
			std::string categoryName{ line.substr(0,comma) }; //category name

			if (category == "Staff") //once staff is hit breaks and starts to read in staff
				break;
		 std::size_t comma{ line.find(',') }; //find break between activity name and activity times available
	  std::string activityName{ line.substr(0,comma) }; //stores activity name
	  line = line.substr(comma + 1, line.size() - comma - 1); //removes activty name from line
	  std::vector < ScheduleSlot* > timesAvailable{};//array storing if activity is available at each time slot

   getScheduleSlots(line, timesAvailable, scheduleSlots); //gets times available from line and adds it to times avaliable vector

	  int timesPerCycle{ std::stoi(line) }; //rest of line after times avaible is times per cycle

	  activities.push_back(Activity(activityName, timesPerCycle, timesAvailable)); //add activity to activities array
  }
}

//adds a schedule slot for each time period per level to the scheduleSlots vector
void assignScheduleSlots(std::vector <ScheduleSlot> &scheduleSlots)
{
 for(std::size_t index {0}; index < periodsInDay * daysInCycle * Level::maxLevel; ++index)
  scheduleSlot.emplace_back(index%(periodsInDay*daysInCycle));
}

//reads in participants and adds them to each schedule slot they are participating in
void readInParticipants(std::ifstream& myReader, std::vector <ScheduleSlot> &scheduleSlots)
{
 std::string line{};//holds line data
 while(std::getline(myReader, line)) //while there are still participants to read in
 {
  std::size_t comma{ line.find(',') };//location of break between participant name and times available
	 line = line.substr(comma + 1, line.size() - comma - 1); //removes participant name from line
	 char groupLevel {line[0]};
  Level level {getLevel(groupLevel)}; //converts level char to Level enum type
  line = line.substr(2, line.size()-2); //removes participant group level from line so it only contains the participant's times available.
  

  std::vector < ScheduleSlot* > timesAvailable{};//array storing if participant is available at each time slot

  getScheduleSlots(line, timesAvailable, scheduleSlots, Level::level*periodsInDay*daysInCycle); //gets times available from line and adds it to times avaliable vector

  for(ScheduleSlot* scheduleSlot: timesAvailable) //adds participant to schedule slots at the times they are participating in their level
   scheduleSlot->addParticipant(timesAvailable); 
 }
}

template <typename t>
void fillFillList(std::vector <T*> spotsToFill, std::vector <T> &spots)
{
 for(std::size_t index{0}; index<scheduleSlots.size(); ++index)
 {
  T* nextSpot {&spots[0]};
  for(std::size_t spotIndex{1}; spotIndex<spots.size(); ++spotIndex)
  {
   if(nextSpot->getTimesLeftToFill()/nextSpot->getTimesToFill()<spots[spotIndex]->getTimesLeftToFill()/spots[spotIndex]->getTimesToFill())
    nextSpot=&spots[spotIndex];
   if(nextSpot->getTimesLeftToFill()==nextSpot->getTimesToFill())
    break;
  }
  spotsToFill.push_back(nextSpot);
  nextActivity.setTimesLeftToFill(nextSpot.getTimesLeftToFill()-1);
 }
}


int main()
{




	//ParticipantGroup testGroup{ 1,timeSlots,50 };
 
 std::vector <ScheduleSlot> scheduleSlots{};
	std::vector <Activity> activities{}; 
	std::vector <Staff> staff{};
 
 assignScheduleSlots(scheduleSlots);

	try
	{
  try
	 {
		 if (!myReader) //if reader fails to open file throw exception
			 throw "File could not be opened\n";
   std::ifstream myReader{ "scheduling.csv" }; //selects file "scheduling.csv" to read in
		 readInActivities(myReader, activities, scheduleSlots); //reads in activities and assigns them to the activities vector
   readInStaff(myReader, activities, staff); //reads in staff
   readInParticipants(myReader, scheduleSlots); //reads in paticipants
	 }
	 catch (const char* errorMessage) //if file could not be opened
	 {
		 std::cerr << errorMessage; //print file error message
		 throw; //rethrow exception
	 }
 }
	catch (...)
	{
		std::cerr << "A fatal error has occured\n";
	}

 //sorts scheduleSlots by most participants
	std::sort(scheduleSlots.begin(), scheduleSlots.end(), [](ScheduleSlot &first, ScheduleSlot &second)
		{
			return first.getNumberOfParticipants() > second.getNumberOfParticipants();
		});

 std::vector <ScheduleSlot*> startOfBlocks{&scheduleSlots[0]}; //holds the starting slot of each block of schedule slots with same number of participants
 std::vector <ParticipantGroup> participantGroups{}; //holds all blocks 
 
 //loop through sorted list of schedule slots
 for(auto &endOfBlock: scheduleSlots)
 {
  //starts new block when number of participants change
  if(endOfBlock.getNumberOfParticipants!= startOfBlocks[startOfBlocks.size()-1]->getNumberOfParticipants)
   startOfBlocks.push_back[&endOfBlock];
 }

 int unfilledSlots{0};

 std::vector <Activity*> activitiesToFill{};
 std::vector <Staff*> staffToFill{};

 fillFillList(activitiesToFill,activities);
 fillFillList(staffToFill,staff);

 //creates participant group blocks and adds them to list
 for(std::size_t index{1}; index<startOfBlocks.size(); ++index)
 {
  participantGroups.emplace_back{ParticipantGroup(startOfBlocks[index-1]-unfilledSlots, startOfBlocks[index]-1, activities, staff,startOfBlocks[index-1]-unfilledSlots-startOfBlocks[0], activitiesToFill, staffToFill)};
  unfilledSlots=participantGroups[participantGroups.size()-1].getUnfilledSlots();
 }

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
