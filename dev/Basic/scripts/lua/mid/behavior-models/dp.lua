--[[
Model - Day Pattern
Type - MNL
Authors - Siyu Li, Harish Loganathan

UPDATED VERSION - Carlos, Adnan, Harish

]]

-- require statements do not work with C++. They need to be commented. The order in which lua files are loaded must be explicitly controlled in C++. 
-- require "Logit"

--Estimated values for all betas
--Note: the betas that not estimated are fixed to zero.
--Tour constants
local beta_tour_work = -16.90
local beta_tour_edu = 10
local beta_tour_shop = -3.70
local beta_tour_others = -0.304

--Intermediate Stops constants
local beta_stop_work = 0
local beta_stop_edu = 0
local beta_stop_shop = 0
local beta_stop_others = 0

--Person type
local beta_parttime_work = 0
local beta_parttime_edu = 0
local beta_parttime_shop = 0
local beta_parttime_others = 0
local beta_selfemployed_work = -1.91
local beta_selfemployed_edu = 0
local beta_selfemployed_shop = -0.0146
local beta_selfemployed_others = 0.0308
local beta_universitystudent_work = 0
local beta_universitystudent_edu = 0
local beta_universitystudent_shop = 0
local beta_universitystudent_others = 0.313
local beta_homemaker_work = 0
local beta_homemaker_edu = 0
local beta_homemaker_shop = 1.25
local beta_homemaker_others = 0.614
local beta_retired_work = 0
local beta_retired_edu = 0
local beta_retired_shop = 1.26
local beta_retired_others = 0.973
local beta_unemployed_work = 0
local beta_unemployed_edu = 0
local beta_unemployed_shop = 1.01
local beta_unemployed_others = 1.11
local beta_nationalservice_work = 0
local beta_nationalservice_edu = 0
local beta_nationalservice_shop = 0
local beta_nationalservice_others = 0
local beta_voluntary_work = 0
local beta_voluntary_edu = 0
local beta_voluntary_shop = 0
local beta_voluntary_others = 0
local beta_domestic_work= 0
local beta_domestic_edu = 0
local beta_domestic_shop = 0
local beta_domestic_others = 0
local beta_otherworker_work = 0
local beta_otherworker_edu = 0
local beta_otherworker_shop = 0
local beta_otherworker_others = 0
local beta_student16_work = 0
local beta_student16_edu = 0
local beta_student16_shop = 0.360
local beta_student16_others = -0.391
local beta_student515_work = 0
local beta_student515_edu = 0
local beta_student515_shop = -0.848
local beta_student515_others = -0.451
local beta_child4_work = 0
local beta_child4_edu = 0
local beta_child4_shop = 0
local beta_child4_others = 0

--Adult age group
local beta_age2025_work = -0.190
local beta_age2025_edu = 0
local beta_age2025_shop = 0.0406
local beta_age2025_others = -0.368
local beta_age2635_work = 0.0449
local beta_age2635_edu = 0
local beta_age2635_shop = 0.0989
local beta_age2635_others = 0
local beta_age5165_work = 0
local beta_age5165_edu = 0
local beta_age5165_shop = 0
local beta_age5165_others = 0

--Adult gender/children
local beta_maleage4_work = -0.705
local beta_maleage4_edu = 0
local beta_maleage4_shop = -0.982
local beta_maleage4_others = -0.808
local beta_maleage515_work = 0.803
local beta_maleage515_edu = 0
local beta_maleage515_shop = -0.536
local beta_maleage515_others = 0.190
local beta_femalenone_work = -1.67
local beta_femalenone_edu = 0
local beta_femalenone_shop = 0.245
local beta_femalenone_others = -0.124
local beta_femaleage4_work = -1.38
local beta_femaleage4_edu = 0
local beta_femaleage4_shop = -0.121
local beta_femaleage4_others = -0.0498
local beta_femaleage515_work = -1.05
local beta_femaleage515_edu = 0
local beta_femaleage515_shop =0.0946 
local beta_femaleage515_others = 0.138

--Household composition
local beta_onlyadults_work = 0.736
local beta_onlyadults_edu = 0
local beta_onlyadults_shop = 0.00896
local beta_onlyadults_others = 0.224
local beta_onlyworkers_work = 0.711
local beta_onlyworkers_edu = 0
local beta_onlyworkers_shop = 0.562
local beta_onlyworkers_others = -0.108

--Personal income
local beta_income_work = 0.373
local beta_income_edu = 0
local beta_income_shop = 0.236
local beta_income_others = 0.187

local beta_missingincome_work = -0.707
local beta_missingincome_edu = 0
local beta_missingincome_shop = -0.0921
local beta_missingincome_others = 0.317

--Others
local beta_workathome_work = 0
local beta_workathome_edu = 0
local beta_workathome_shop = 0
local beta_workathome_others = 0
local beta_caravail_work = -1.41
local beta_caravail_edu = 0
local beta_caravail_shop = 0.168
local beta_caravail_others = 1.21
local beta_motoravail_work = 0.413
local beta_motoravail_edu = 0
local beta_motoravail_shop = 0.142
local beta_motoravail_others = -0.207

--Additional constants
local beta_onetour_onestop = 0
local beta_onetour_twostop = -2.42
local beta_onetour_threestop = 0
local beta_twotour_onestop = 0
local beta_twotour_twostop = 0
local beta_twotour_threestop = 0
local beta_threetour_onestop = 0
local beta_threetour_twostop = 0

local beta_work_logsum = 1.48
local beta_edu_logsum = 0
local beta_shopping_logsum = 0.00595
local beta_other_logsum = -0.348

--Combination constants
local beta_workwork_ts = 0
local beta_workshop_tt = -2.67
local beta_workshop_ts = -4.89
local beta_workothers_tt = -1.84
local beta_workothers_ss = 0
local beta_workothers_ts = 0
local beta_eduedu_ts =	0
local beta_edushop_tt =	-1.60
local beta_edushop_ts = -4.49
local beta_eduothers_tt = -1.56
local beta_eduothers_ss = 0
local beta_eduothers_ts = 0
local beta_shopshop_ts = -3.47
local beta_shopothers_tt = -0.816
local beta_shopothers_ss = 2.44
local beta_shopothers_ts = 0
local beta_othersothers_ts = -1.68
local beta_othersshop_ts =	-4.37

--choiceset
local choice = {
        {0,0,0,0,0,0,0,0}, 
        {0,0,0,1,0,0,0,0}, 
        {0,0,0,1,0,0,0,1}, 
        {0,0,0,1,0,0,1,0}, 
        {0,0,0,1,0,0,1,1}, 
        {0,0,1,0,0,0,0,0}, 
        {0,0,1,0,0,0,0,1}, 
        {0,0,1,0,0,0,1,0}, 
        {0,0,1,0,0,0,1,1}, 
        {0,0,1,1,0,0,0,0}, 
        {0,0,1,1,0,0,0,1}, 
        {0,0,1,1,0,0,1,0}, 
        {0,1,0,0,0,0,0,0}, 
        {0,1,0,0,0,0,0,1}, 
        {0,1,0,0,0,0,1,0}, 
        {0,1,0,0,0,0,1,1}, 
        {0,1,0,0,0,1,0,0}, 
        {0,1,0,0,0,1,0,1}, 
        {0,1,0,0,0,1,1,0}, 
        {0,1,0,1,0,0,0,0}, 
        {0,1,0,1,0,0,0,1}, 
        {0,1,0,1,0,0,1,0}, 
        {0,1,0,1,0,1,0,0}, 
        {0,1,1,0,0,0,0,0}, 
        {0,1,1,0,0,0,0,1}, 
        {0,1,1,0,0,0,1,0}, 
        {0,1,1,0,0,1,0,0}, 
        {0,1,1,1,0,0,0,0}, 
        {1,0,0,0,0,0,0,0}, 
        {1,0,0,0,0,0,0,1}, 
        {1,0,0,0,0,0,1,0}, 
        {1,0,0,0,0,0,1,1}, 
        {1,0,0,0,1,0,0,0}, 
        {1,0,0,0,1,0,0,1}, 
        {1,0,0,0,1,0,1,0}, 
        {1,0,0,1,0,0,0,0}, 
        {1,0,0,1,0,0,0,1}, 
        {1,0,0,1,0,0,1,0}, 
        {1,0,0,1,1,0,0,0}, 
        {1,0,1,0,0,0,0,0}, 
        {1,0,1,0,0,0,0,1}, 
        {1,0,1,0,0,0,1,0}, 
        {1,0,1,0,1,0,0,0}, 
        {1,0,1,1,0,0,0,0}, 
        {1,1,0,0,0,0,0,0}, 
        {1,1,0,0,0,0,0,1}, 
        {1,1,0,0,0,0,1,0}, 
        {1,1,0,0,0,1,0,0}, 
        {1,1,0,0,1,0,0,0}, 
        {1,1,0,1,0,0,0,0}, 
        {1,1,1,0,0,0,0,0}
}

  -- WorkTi,EduTi,ShopTi,OthersTi, WorkIi,EduIi,ShopIi,OthersIi
local WorkT = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
local EduT = {0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1}
local ShopT = {0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,1}
local OthersT = {0,1,1,1,1,0,0,0,0,1,1,1,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,1,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,1,0,0,0,0,0,1,0}
local WorkI = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,1,0,0,0,1,0,0,0,0,0,1,0,0}
local EduI = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0}
local ShopI = {0,0,0,1,1,0,0,1,1,0,0,1,0,0,1,1,0,0,1,0,0,1,0,0,0,1,0,0,0,0,1,1,0,0,1,0,0,1,0,0,0,1,0,0,0,0,1,0,0,0,0}
local OthersI = {0,0,1,0,1,0,1,0,1,0,1,0,0,1,0,1,0,1,0,0,1,0,0,0,1,0,0,0,0,1,0,1,0,1,0,0,1,0,0,0,1,0,0,0,0,1,0,0,0,0,0}

  -- XXXtour_XXXstop. . . series
local onetour_onestop = {0,0,1,1,0,0,1,1,0,0,0,0,0,1,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
local onetour_twostop = {0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,1,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
local onetour_threestop = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
local twotour_onestop = {0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,1,1,1,0,1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,0,1,1,1,0,0,1,1,1,1,0,0}
local twotour_twostop = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
local twotour_threestop = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
local threetour_onestop = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
local threetour_twostop = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}

  -- worki,edui,shopi,otheri series 
local work = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
local edu = {0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1}
local shop = {0,0,0,1,1,1,1,1,1,1,1,1,0,0,1,1,0,0,1,0,0,1,0,1,1,1,1,1,0,0,1,1,0,0,1,0,0,1,0,1,1,1,1,1,0,0,1,0,0,0,1}
local other = {0,1,1,1,1,0,1,0,1,1,1,1,0,1,0,1,0,1,0,1,1,1,1,0,1,0,0,1,0,1,0,1,0,1,0,1,1,1,1,0,1,0,0,1,0,1,0,0,0,1,0}

--XXX_tt_ss_ts .. series
local workwork_ts = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,1,0,0,0,1,0,0,0,0,0,1,0,0}
local workshop_tt = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,1}
local workshop_ts = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,0,0,1,0,0,0,1,0,0,0,0,1,0,0,0,0}
local workothers_tt = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,1,0,0,0,0,0,1,0}
local workothers_ss = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
local workothers_ts = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,0,1,0,0,0,1,0,0,0,0,1,0,0,0,0,0}

local eduedu_ts = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0}
local edushop_tt = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}
local edushop_ts = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0}
local eduothers_tt = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0}
local eduothers_ss = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
local eduothers_ts = {0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0}

local shopshop_ts = {0,0,0,0,0,0,0,1,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0}
local shopothers_tt = {0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0}
local shopothers_ss = {0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
local shopothers_ts = {0,0,0,0,0,0,1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0}

local othersothers_ts = {0,0,1,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
local othersshop_ts = {0,0,0,1,1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0}

--utility
local utility = {}
local function computeUtilities(params) 
	-- storing data from params table passed into this function locally for use in this function (this is purely for better execution time)
	local pid = params.person_id
	local person_type_id = params.person_type_id 
	local age_id = params.age_id
	local universitystudent = params.universitystudent
	local onlyadults = params.only_adults
	local onlyworkers = params.only_workers
	local HH_with_under_4 = params.num_underfour
	local HH_with_under_15 = params.presence_of_under15
	local female_dummy = params.female_dummy
	local income_id = params.income_id
	local income_mid = {500.5,1250,1749.5,2249.5,2749.5,3499.5,4499.5,5499.5,6499.5,7499.5,8500,0,99999,99999}
	local missing_income = params.missing_income
	local workathome = params.work_at_home_dummy
	local car_own = params.car_own
	local car_own_normal = params.car_own_normal
	local car_own_offpeak = params.car_own_offpeak
	local motor_own = params.motor_own
	local worklogsum = params.worklogsum
	local edulogsum = params.edulogsum
	local shoplogsum = params.shoplogsum
	local otherlogsum = params.otherlogsum

	-- person type related variables
	local fulltime,parttime,selfemployed,homemaker,retired,univ_student,unemployed,nationalservice,voluntary,domestic,otherworker,student16,student515,child4 = 0,0,0,0,0,0,0,0,0,0,0,0,0,0	
	if person_type_id == 1 then
		fulltime = 1
	elseif person_type_id == 2 then 
		parttime = 1
	elseif person_type_id == 3 then
		selfemployed = 1
	elseif person_type_id == 4 and universitystudent == 1 then 
		univ_student = 1
	elseif person_type_id == 5 then
		homemaker = 1
	elseif person_type_id == 6 then
		retired = 1
	elseif person_type_id == 7 then
		unemployed = 1
	elseif person_type_id == 8 then
		nationalservice = 1
	elseif person_type_id == 9 then
		voluntary = 1
	elseif person_type_id == 10 then
		domestic = 1
	elseif person_type_id == 12 then
		otherworker = 1
	end 
	if person_type_id == 4 and age_id == 3 then 
		student16 = 1
	elseif person_type_id == 4 and (age_id == 1 or age_id == 2) then
		student515 = 1
	end
	if age_id == 0 then 
		child4 = 1 
	end

	-- age group related variables
	local age20,age2025,age2635,age3650,age5165,age65 = 0,0,0,0,0,0
	if age_id < 4 then 
		age20 = 1
	elseif age_id == 4 then 
		age2025 = 1
	elseif age_id == 5 or age_id == 6 then 
		age2635 = 1
	elseif age_id == 7 or age_id == 8 or age_id == 9 then 
		age3650 = 1
	elseif age_id == 10 or age_id == 11 or age_id == 12 then 
		age5165 = 1
	elseif age_id > 12 then 
		age65 = 1
	end

	-- Adult gender/children related variables
	--HH_with_under_4 is the number
	--However, HH_with_under_15 is binary
	local maleage4,maleage515,malenone,femalenone,femaleage4,femaleage515 = 0,0,0,0,0,0
	if female_dummy == 0 then
		if HH_with_under_4 >= 1 then 
			maleage4 = 1 
		end
		if HH_with_under_4 == 0 and HH_with_under_15 == 1 then 
			maleage515 = 1 
		end
		if onlyadults == 1 then 
			malenone = 1 
		end
	elseif female_dummy == 1 then
		if HH_with_under_4 >= 1 then 
			femaleage4 = 1 
		end
		if HH_with_under_4 == 0 and HH_with_under_15 == 1 then 
			femaleage515 = 1 
		end
		if onlyadults == 1 then 
			femalenone = 1 
		end
	end

	-- income related variables
	local income = income_mid[income_id] * (1 - missing_income)/1000 

	-- other variables
	local caravail,motoravail = 0,0
	if car_own >= 1  then 
		caravail = 1 
	end
	if motor_own >= 1 then 
		motoravail = 1 
	end
			
	for i = 1,51 do
		utility[i] = 
			beta_tour_work * WorkT[i] + 
			beta_stop_work * WorkI[i] +
			beta_tour_edu * EduT[i] + 
			beta_stop_edu * EduI[i] +
			beta_tour_shop * ShopT[i] + 
			beta_stop_shop * ShopI[i] +
			beta_tour_others * OthersT[i] + 
			beta_stop_others * OthersI[i] +
			beta_onetour_onestop * onetour_onestop[i] +
			beta_onetour_twostop * onetour_twostop[i] +
			beta_onetour_threestop * onetour_threestop[i] +
			beta_twotour_onestop * twotour_onestop[i] +
			beta_twotour_twostop * twotour_twostop[i] +
			beta_twotour_threestop * twotour_threestop[i] +
			beta_threetour_onestop * threetour_onestop[i] +
			beta_threetour_twostop * threetour_twostop[i] +
			beta_parttime_work * (work[i] * parttime) +
			beta_parttime_edu * (edu[i] * parttime) +
			beta_parttime_shop * (shop[i] * parttime) +
			beta_parttime_others * (other[i] * parttime) +
			beta_selfemployed_work * (work[i] * selfemployed) +
			beta_selfemployed_edu * (edu[i] * selfemployed) +
			beta_selfemployed_shop * (shop[i] * selfemployed) +
			beta_selfemployed_others * (other[i] * selfemployed) +
			beta_universitystudent_work * (work[i] * univ_student) +
			beta_universitystudent_edu * (edu[i] * univ_student) +
			beta_universitystudent_shop * (shop[i] * univ_student) +
			beta_universitystudent_others * (other[i] * univ_student) +
			beta_homemaker_work * (work[i] * homemaker) +
			beta_homemaker_edu * (edu[i] * homemaker) +
			beta_homemaker_shop * (shop[i] * homemaker) +
			beta_homemaker_others * (other[i] * homemaker) +
			beta_retired_work * (work[i] * retired) +
			beta_retired_edu * (edu[i] * retired) +
			beta_retired_shop * (shop[i] * retired) +
			beta_retired_others * (other[i] * retired) +
			beta_unemployed_work * (work[i] * unemployed) +
			beta_unemployed_edu * (edu[i] * unemployed) +
			beta_unemployed_shop * (shop[i] * unemployed) +
			beta_unemployed_others * (other[i] * unemployed) +
			beta_nationalservice_work * (work[i] * nationalservice) +
			beta_nationalservice_edu * (edu[i] * nationalservice) +
			beta_nationalservice_shop * (shop[i] * nationalservice) +
			beta_nationalservice_others * (other[i] * nationalservice) +
			beta_voluntary_work * (work[i] * voluntary) +
			beta_voluntary_edu * (edu[i] * voluntary) +
			beta_voluntary_shop * (shop[i] * voluntary) +
			beta_voluntary_others * (other[i] * voluntary) +
			beta_domestic_work * (work[i] * domestic) +
			beta_domestic_edu * (edu[i] * domestic) +
			beta_domestic_shop * (shop[i] * domestic) +
			beta_domestic_others * (other[i] * domestic) +
			beta_otherworker_work * (work[i] * otherworker) +
			beta_otherworker_edu * (edu[i] * otherworker) +
			beta_otherworker_shop * (shop[i] * otherworker) +
			beta_otherworker_others * (other[i] * otherworker) +
			beta_student16_work * (work[i] * student16) +
			beta_student16_edu * (edu[i] * student16) +
			beta_student16_shop * (shop[i] * student16) +
			beta_student16_others * (other[i] * student16) +
			beta_student515_work * (work[i] * student515) +
			beta_student515_edu * (edu[i] * student515) +
			beta_student515_shop * (shop[i] * student515) +
			beta_student515_others * (other[i] * student515) +
			beta_child4_work * (work[i] * child4) +
			beta_child4_edu * (edu[i] * child4) +
			beta_child4_shop * (shop[i] * child4) +
			beta_child4_others * (other[i] * child4) +
			beta_age2025_work * (work[i] * age2025) +
			beta_age2025_edu * (edu[i] * age2025) +
			beta_age2025_shop * (shop[i] * age2025) +
			beta_age2025_others * (other[i] * age2025) +
			beta_age2635_work * (work[i] * age2635) +
			beta_age2635_edu * (edu[i] * age2635) +
			beta_age2635_shop * (shop[i] * age2635) +
			beta_age2635_others * (other[i] * age2635) +
			beta_age5165_work * (work[i] * age5165) +
			beta_age5165_edu * (edu[i] * age5165) +
			beta_age5165_shop * (shop[i] * age5165) +
			beta_age5165_others * (other[i] * age5165) +
			beta_maleage4_work * (work[i] * maleage4) +
			beta_maleage4_edu * (edu[i] * maleage4) +
			beta_maleage4_shop * (shop[i] * maleage4) +
			beta_maleage4_others * (other[i] * maleage4) +
			beta_maleage515_work * (work[i] * maleage515) +
			beta_maleage515_edu * (edu[i] * maleage515) +
			beta_maleage515_shop * (shop[i] * maleage515) +
			beta_maleage515_others * (other[i] * maleage515) +
			beta_femalenone_work * (work[i] * femalenone) +
			beta_femalenone_edu * (edu[i] * femalenone) +
			beta_femalenone_shop * (shop[i] * femalenone) +
			beta_femalenone_others * (other[i] * femalenone) +
			beta_femaleage4_work * (work[i] * femaleage4) +
			beta_femaleage4_edu * (edu[i] * femaleage4) +
			beta_femaleage4_shop * (shop[i] * femaleage4) +
			beta_femaleage4_others * (other[i] * femaleage4) +
			beta_femaleage515_work * (work[i] * femaleage515) +
			beta_femaleage515_edu * (edu[i] * femaleage515) +
			beta_femaleage515_shop * (shop[i] * femaleage515) +
			beta_femaleage515_others * (other[i] * femaleage515) +
			beta_onlyadults_work * (work[i] * onlyadults) +
			beta_onlyadults_edu * (edu[i] * onlyadults) +
			beta_onlyadults_shop * (shop[i] * onlyadults) +
			beta_onlyadults_others * (other[i] * onlyadults) +
			beta_onlyworkers_work * (work[i] * onlyworkers) +
			beta_onlyworkers_edu * (edu[i] * onlyworkers) +
			beta_onlyworkers_shop * (shop[i] * onlyworkers) +
			beta_onlyworkers_others * (other[i] * onlyworkers) +
			beta_income_work * (work[i] * income) +
			beta_income_edu * (edu[i] * income) +
			beta_income_shop * (shop[i] * income) +
			beta_income_others * (other[i] * income) +
			beta_missingincome_work * (work[i] * missing_income) + 
			beta_missingincome_edu * (edu[i] * missing_income) + 
			beta_missingincome_shop * (shop[i] * missing_income) + 
			beta_missingincome_others * (other[i] * missing_income) + 
			beta_workathome_work * (work[i] * workathome) +
			beta_workathome_edu * (edu[i] * workathome) +
			beta_workathome_shop * (shop[i] * workathome) +
			beta_workathome_others * (other[i] * workathome) +
			beta_caravail_work * (work[i] * caravail) +
			beta_caravail_edu * (edu[i] * caravail) +
			beta_caravail_shop * (shop[i] * caravail) +
			beta_caravail_others * (other[i] * caravail) +
			beta_motoravail_work * (work[i] * motoravail) +
			beta_motoravail_edu * (edu[i] * motoravail) +
			beta_motoravail_shop * (shop[i] * motoravail) +
			beta_motoravail_others * (other[i] * motoravail) +
			beta_work_logsum * WorkT[i] * worklogsum +
			beta_edu_logsum * EduT[i] * edulogsum +
			beta_shopping_logsum * ShopT[i] * shoplogsum +
			beta_other_logsum * OthersT[i] * otherlogsum +
			beta_workwork_ts * workwork_ts[i] +
			beta_workshop_tt * workshop_tt[i] +
			beta_workshop_ts * workshop_ts[i] +
			beta_workothers_tt * workothers_tt[i] +
			beta_workothers_ss * workothers_ss[i] +
			beta_workothers_ts * workothers_ts[i] +
			beta_eduedu_ts * eduedu_ts[i] +
			beta_edushop_tt * edushop_tt[i] +
			beta_edushop_ts * edushop_ts[i] +
			beta_eduothers_tt * eduothers_tt[i] +
			beta_eduothers_ss * eduothers_ss[i] +
			beta_eduothers_ts * eduothers_ts[i] +
			beta_shopshop_ts * shopshop_ts[i] +
			beta_shopothers_tt * shopothers_tt[i] +
			beta_shopothers_ss * shopothers_ss[i] +
			beta_shopothers_ts * shopothers_ts[i] +
			beta_othersothers_ts * othersothers_ts[i] +
			beta_othersshop_ts * othersshop_ts[i]
	end
end

--availability
local availability = {}
local function computeAvailabilities(params)
	-- storing data from params table passed into this function locally for use in this function (this is purely for better execution time)
	local person_type_id = params.person_type_id

	for i = 1,51 do
		-- For Full time student (person_type_id=4): All alternatives are available.
		-- For other person type: only alternatives with EduT=0 (i.e. choice[i][2] = 0) are available to them
		if person_type_id == 4 then
			availability[i] = 1
		else
			if choice[i][2] == 0 then 
				availability[i] = 1
			else
				availability[i] = 0
			end
		end
	end
end

-- scales
local scale = 1 --for all choices

-- function to call from C++ preday simulator
-- params table contains data passed from C++
-- to check variable bindings in params, refer PredayLuaModel::mapClasses() function in dev/Basic/medium/behavioral/lua/PredayLuaModel.cpp
function choose_dp(params)
	computeUtilities(params) 
	computeAvailabilities(params)
	local probability = calculate_probability("mnl", choice, utility, availability, scale)
	idx = make_final_choice(probability)
	return choice[idx]
end
