--[[
Model - Mode/destination choice for work tour to unusual location
Type - logit
Authors - Siyu Li, Harish Loganathan
]]

-- all require statements do not work with C++. They need to be commented. The order in which lua files are loaded must be explicitly controlled in C++. 
--require "Logit"

--Estimated values for all betas
--Note: the betas that not estimated are fixed to zero.

--!! see the documentation on the definition of AM,PM and OP table!!

local beta_cost_bus_mrt_1= -6.75
local beta_cost_private_bus_1 = -7.81
local beta_cost_drive1_1 = -5.90
local beta_cost_share2_1 = -8.80
local beta_cost_share3_1 = -3.39
local beta_cost_motor_1 = -0.553
local beta_cost_taxi_1 = -1.49
local beta_cost_SMS_1 = -1.69
local beta_cost_Rail_SMS_1 = -6.75
local beta_cost_SMS_Pool_1 = -1.49
local beta_cost_Rail_SMS_Pool_1 = -6.75
local beta_cost_AMOD_1 = -1.69
local beta_cost_Rail_AMOD_1 = -6.75
local beta_cost_AMOD_Pool_1 = -1.49
local beta_cost_Rail_AMOD_Pool_1 = -6.75

local beta_cost_bus_mrt_2 = 0
local beta_cost_private_bus_2 = 0
local beta_cost_drive1_2 = 0
local beta_cost_share2_2 = 0
local beta_cost_share3_2 = 0
local beta_cost_motor_2 = 0
local beta_cost_taxi_2 = 0
local beta_cost_SMS_2 = 0
local beta_cost_Rail_SMS_2 = 0
local beta_cost_SMS_Pool_2 = 0
local beta_cost_Rail_SMS_Pool_2 = 0
local beta_cost_AMOD_2 = 0
local beta_cost_Rail_AMOD_2 = 0
local beta_cost_AMOD_Pool_2 = 0
local beta_cost_Rail_AMOD_Pool_2 = 0

local beta_tt_bus_mrt = -1.04
local beta_tt_private_bus= 0
local beta_tt_drive1 = -0.470
local beta_tt_share2 = 0
local beta_tt_share3 = 0
local beta_tt_motor = 0
local beta_tt_walk = -3.01
local beta_tt_taxi= -1.71
local beta_tt_SMS = -1.71
local beta_tt_Rail_SMS = -1.04
local beta_tt_SMS_Pool = -1.71
local beta_tt_Rail_SMS_Pool = -1.04
local beta_tt_AMOD = -1.71
local beta_tt_Rail_AMOD = -1.04
local beta_tt_AMOD_Pool = -1.71
local beta_tt_Rail_AMOD_Pool = -1.04

local beta_log = 0.793
local beta_area = 5.61
local beta_population = 0

local beta_central_bus_mrt = 0.284
local beta_central_private_bus = -1.43
local beta_central_drive1 = 0
local beta_central_share2 = 0.226
local beta_central_share3 = 0.367
local beta_central_motor = 0.618
local beta_central_walk = -1.32
local beta_central_taxi = 1.13
local beta_central_SMS = 1.13
local beta_central_Rail_SMS = 0.284
local beta_central_SMS_Pool = 1.13
local beta_central_Rail_SMS_Pool = 0.284
local beta_central_AMOD = 1.13
local beta_central_Rail_AMOD = 0.284
local beta_central_AMOD_Pool = 1.13
local beta_central_Rail_AMOD_Pool = 0.284

local beta_distance_bus_mrt = 0
local beta_distance_private_bus = 0
local beta_distance_drive1 = 0
local beta_distance_share2 = -0.018
local beta_distance_share3 = -0.0299
local beta_distance_motor = -0.0319
local beta_distance_walk = 0
local beta_distance_taxi = 0
local beta_distance_SMS =0
local beta_distance_Rail_SMS = 0
local beta_distance_SMS_Pool =0
local beta_distance_Rail_SMS_Pool = 0
local beta_distance_AMOD =0
local beta_distance_Rail_AMOD = 0
local beta_distance_AMOD_Pool =0
local beta_distance_Rail_AMOD_Pool = 0

local beta_cons_bus = -7.182
local beta_cons_mrt = -6.948
local beta_cons_private_bus = -7.866
local beta_cons_drive1 = 0
local beta_cons_share2 = -10.164
local beta_cons_share3 = -11.705
local beta_cons_motor = -8.018
local beta_cons_walk = -7.594
local beta_cons_taxi = -11.135
local beta_cons_SMS = -7.513
local beta_cons_Rail_SMS = -12.630
local beta_cons_SMS_Pool = -10.259
local beta_cons_Rail_SMS_Pool =  -21.025
local beta_cons_AMOD = -7.513
local beta_cons_Rail_AMOD = -12.630
local beta_cons_AMOD_Pool = -10.259
local beta_cons_Rail_AMOD_Pool =  -21.025

local beta_zero_bus = 0
local beta_oneplus_bus = -3.20
local beta_twoplus_bus = -0.658
local beta_threeplus_bus = 0

local beta_zero_mrt= 0
local beta_oneplus_mrt = -2.53
local beta_twoplus_mrt = -1.71
local beta_threeplus_mrt = 0

local beta_zero_privatebus = 0
local beta_oneplus_privatebus = -3.54
local beta_twoplus_privatebus = 0
local beta_threeplus_privatebus = 0

local beta_zero_drive1 = 0
local beta_oneplus_drive1 = 0
local beta_twoplus_drive1 = 0.558
local beta_threeplus_drive1 = 0

local beta_zero_share2 = 0
local beta_oneplus_share2 = 0.502
local beta_twoplus_share2 = 0
local beta_threeplus_share2 = 0

local beta_zero_share3 = 0
local beta_oneplus_share3 = -0.515
local beta_twoplus_share3 = 0
local beta_threeplus_share3 = 0

local beta_zero_car_motor = 0
local beta_oneplus_car_motor = -3.12
local beta_twoplus_car_motor = 0
local beta_threeplus_car_motor = 0

local beta_zero_walk = 0
local beta_oneplus_walk = -2.65
local beta_twoplus_walk = 0
local beta_threeplus_walk = 0

local beta_zero_taxi = 0
local beta_oneplus_taxi = -3.03
local beta_twoplus_taxi = 0
local beta_threeplus_taxi = 0

local beta_zero_SMS = 0
local beta_oneplus_SMS = -3.03
local beta_twoplus_SMS = 0
local beta_threeplus_SMS = 0

local beta_zero_SMS_Pool = 0
local beta_oneplus_SMS_Pool = -3.03
local beta_twoplus_SMS_Pool = 0
local beta_threeplus_SMS_Pool = 0

local beta_zero_Rail_SMS = 0
local beta_oneplus_Rail_SMS = -2.53
local beta_twoplus_Rail_SMS = -1.71
local beta_threeplus_Rail_SMS = 0

local beta_zero_Rail_SMS_Pool = 0
local beta_oneplus_Rail_SMS_Pool = -2.53
local beta_twoplus_Rail_SMS_Pool = -1.71
local beta_threeplus_Rail_SMS_Pool = 0

local beta_zero_AMOD = 0
local beta_oneplus_AMOD = -3.03
local beta_twoplus_AMOD = 0
local beta_threeplus_AMOD = 0

local bundled_variables = {}
bundled_variables.beta_zero_AMOD_Pool = 0
bundled_variables.beta_oneplus_AMOD_Pool = -3.03
bundled_variables.beta_twoplus_AMOD_Pool = 0
bundled_variables.beta_threeplus_AMOD_Pool = 0

bundled_variables.beta_zero_Rail_AMOD = 0
bundled_variables.beta_oneplus_Rail_AMOD = -2.53
bundled_variables.beta_twoplus_Rail_AMOD = -1.71
bundled_variables.beta_threeplus_Rail_AMOD = 0

bundled_variables.beta_zero_Rail_AMOD_Pool = 0
bundled_variables.beta_oneplus_Rail_AMOD_Pool = -2.53
bundled_variables.beta_twoplus_Rail_AMOD_Pool = -1.71
bundled_variables.beta_threeplus_Rail_AMOD_Pool = 0

bundled_variables.beta_zero_motor = 0
bundled_variables.beta_oneplus_motor = 0
bundled_variables.beta_twoplus_motor = 4.79
bundled_variables.beta_threeplus_motor= 0

bundled_variables.beta_female_bus = 1.94
bundled_variables.beta_female_mrt = 1.60
bundled_variables.beta_female_private_bus = 1.41
bundled_variables.beta_female_drive1 = 0
bundled_variables.beta_female_share2 = 1.72
bundled_variables.beta_female_share3 = 0.429
bundled_variables.beta_female_motor = -2.36
bundled_variables.beta_female_taxi = 2.89
bundled_variables.beta_female_walk = 3.08
bundled_variables.beta_female_Rail_SMS = 1.60
bundled_variables.beta_female_SMS = 2.89
bundled_variables.beta_female_Rail_SMS_Pool = 1.60
bundled_variables.beta_female_SMS_Pool = 2.89
bundled_variables.beta_female_Rail_AMOD = 1.60
bundled_variables.beta_female_AMOD = 2.89
bundled_variables.beta_female_Rail_AMOD_Pool = 1.60
bundled_variables.beta_female_AMOD_Pool = 2.89


--choice set
local choice = {}

for i = 1, 1169*16 do 

	choice[i] = i
end

--utility
-- 1 for public bus; 2 for MRT/LRT; 3 for private bus; 4 for drive1;
-- 5 for shared2; 6 for shared3+; 7 for motor; 8 for walk; 9 for taxi
local utility = {}
local function computeUtilities(params,dbparams)
	local cost_increase = dbparams.cost_increase
	local female_dummy = params.female_dummy
	local income_id = params.income_id
	local income_cat = {500,1250,1750,2250,2750,3500,4500,5500,6500,7500,8500,0,99999,99999}
	local income_mid = income_cat[income_id]

	local missing_income = (params.income_id >= 13) and 1 or 0


	local zero_car,one_plus_car,two_plus_car,three_plus_car, zero_motor,one_plus_motor,two_plus_motor,three_plus_motor = 0,0,0,0,0,0,0,0
	local veh_own_cat = params.vehicle_ownership_category
	if veh_own_cat == 0 or veh_own_cat == 1 or veh_own_cat == 2 then 
		zero_car = 1 
	
	end
	if veh_own_cat == 3 or veh_own_cat == 4 or veh_own_cat == 5  then 
		one_plus_car = 1 
	end
	if veh_own_cat == 5  then 
		two_plus_car = 1 
	end
	
	if veh_own_cat == 5  then 
		three_plus_car = 1 
	end
	if veh_own_cat == 0 or veh_own_cat == 3  then 
		zero_motor = 1 
	end
	if veh_own_cat == 1 or veh_own_cat == 2 or veh_own_cat == 4 or veh_own_cat == 5  then 
		one_plus_motor = 1 
	end
	
	if veh_own_cat == 1 or veh_own_cat == 2 or veh_own_cat == 4 or veh_own_cat == 5  then 
		two_plus_motor = 1 
	end
	
	if veh_own_cat == 1 or veh_own_cat == 2 or veh_own_cat == 4 or veh_own_cat == 5  then 
		three_plus_motor = 1 
	end


	local cost_public_first = {}
	local cost_public_second = {}
	local cost_bus = {}
	local cost_mrt = {}
	local cost_Rail_mrt = {}
	local cost_Rail_SMS = {}
	local cost_Rail_SMS_AE_1 = {}
	local cost_Rail_SMS_AE_2 = {} 
	local cost_Rail_SMS_AE_Pool_1 = {}
	local cost_Rail_SMS_AE_Pool_2 = {}
	local cost_Rail_SMS_AE_avg = {}
	local cost_Rail_SMS_Pool = {}
	local cost_Rail_SMS_AE_Pool_avg = {}
	local cost_private_bus = {}
	local cost_Rail_AMOD = {}
	local cost_Rail_AMOD_AE_1 = {}
	local cost_Rail_AMOD_AE_2 = {} 
	local cost_Rail_AMOD_AE_Pool_1 = {}
	local cost_Rail_AMOD_AE_Pool_2 = {}
	local cost_Rail_AMOD_AE_avg = {}
	local cost_Rail_AMOD_Pool = {}
	local cost_Rail_AMOD_AE_Pool_avg = {}

	local cost_car_OP_first = {}
	local cost_car_OP_second = {}
	local cost_car_ERP_first = {}
	local cost_car_ERP_second = {}
	local cost_car_parking = {}
	local cost_drive1 = {}
	local cost_share2 = {}
	local cost_share3 = {}
	local cost_motor = {}
	
	local cost_taxi_1 = {}
	local cost_taxi_2 = {}
	local cost_taxi={}
	
	local cost_SMS_1 = {}
	local cost_SMS_2 = {}
	local cost_SMS={}
	local cost_SMS_Pool_1 = {}
	local cost_SMS_Pool_2 = {}
	local cost_SMS_Pool={}
	
	local cost_AMOD_1 = {}
	local cost_AMOD_2 = {}
	local cost_AMOD={}
	local cost_AMOD_Pool_1 = {}
	local cost_AMOD_Pool_2 = {}
	local cost_AMOD_Pool={}
	
	
	local d1={}
	local d2={}
	local central_dummy={}

	local cost_over_income_bus = {}
	local cost_over_income_mrt = {}
	local cost_over_income_private_bus = {}
	local cost_over_income_drive1 = {}
	local cost_over_income_share2 = {}
	local cost_over_income_share3 = {}
	local cost_over_income_motor = {}
	local cost_over_income_taxi = {}
	local cost_over_income_SMS = {}
	local cost_over_income_Rail_SMS = {}
	local cost_over_income_SMS_Pool = {}
	local cost_over_income_Rail_SMS_Pool = {}
	local cost_over_income_AMOD = {}
	local cost_over_income_Rail_AMOD = {}
	local cost_over_income_AMOD_Pool = {}
	local cost_over_income_Rail_AMOD_Pool = {}

	local tt_public_ivt_first = {}
	local tt_public_ivt_second = {}
	local tt_public_out_first = {}
	local tt_public_out_second = {}
	local tt_car_ivt_first = {}
	local tt_car_ivt_second = {}

	local tt_bus = {}
	local tt_mrt = {}
	local tt_private_bus = {}
	local tt_drive1 = {}
	local tt_share2 = {}
	local tt_share3 = {}
	local tt_motor = {}
	local tt_walk = {}
	local tt_taxi = {}
	local tt_SMS = {}
	local tt_Rail_SMS = {}
	local tt_SMS_Pool = {}
	local tt_Rail_SMS_Pool = {}
	local tt_AMOD = {}
	local tt_Rail_AMOD = {}
	local tt_AMOD_Pool = {}
	local tt_Rail_AMOD_Pool = {}

	local average_transfer_number = {}

	local employment = {}
	local population = {}
	local area = {}
	local shop = {}
	
	

	local log = math.log
	local exp = math.exp

	for i =1,1169 do
		d1[i] = dbparams:walk_distance1(i)
		d2[i] = dbparams:walk_distance2(i)


		--dbparams:cost_public_first(i) = AM[(origin,destination[i])]['pub_cost']
		--dbparams:cost_public_second(i) = PM[(destination[i],origin)]['pub_cost']
		--origin is home, destination(i) is zone from 1 to 1169

		--0 if origin == destination
		
	
		
		cost_public_first[i] = dbparams:cost_public_first(i)
		cost_public_second[i] = dbparams:cost_public_second(i)
		cost_bus[i] = cost_public_first[i] + cost_public_second[i] + cost_increase
		cost_mrt[i] = cost_public_first[i] + cost_public_second[i] + cost_increase
		
		cost_private_bus[i] = cost_public_first[i] + cost_public_second[i] + cost_increase

		--dbparams:cost_car_ERP_first(i) = AM[(origin,destination[i])]['car_cost_erp']
		--dbparams:cost_car_ERP_second(i) = PM[(destination[i],origin)]['car_cost_erp']
		--dbparams:cost_car_OP_first(i) = AM[(origin,destination[i])]['distance']*0.147
		--dbparams:cost_car_OP_second(i) = PM[(destination[i],origin)]['distance']*0.147
		--dbparams:cost_car_parking(i) = 8 * ZONE[destination[i]]['parking_rate']
		--for the above 5 variables, origin is home, destination[i] is tour destination from 1 to 1169
		--0 if origin == destination
		cost_drive1[i] = dbparams:cost_car_ERP_first(i)+dbparams:cost_car_ERP_second(i)+dbparams:cost_car_OP_first(i)+dbparams:cost_car_OP_second(i)+dbparams:cost_car_parking(i)+cost_increase
		cost_share2[i] = (dbparams:cost_car_ERP_first(i)+dbparams:cost_car_ERP_second(i)+dbparams:cost_car_OP_first(i)+dbparams:cost_car_OP_second(i)+dbparams:cost_car_parking(i)+cost_increase)/2
		cost_share3[i] = (dbparams:cost_car_ERP_first(i)+dbparams:cost_car_ERP_second(i)+dbparams:cost_car_OP_first(i)+dbparams:cost_car_OP_second(i)+dbparams:cost_car_parking(i)+cost_increase)/3
		cost_motor[i] = 0.5*(dbparams:cost_car_ERP_first(i)+dbparams:cost_car_ERP_second(i)+dbparams:cost_car_OP_first(i)+dbparams:cost_car_OP_second(i))+0.65*dbparams:cost_car_parking(i)+cost_increase

		--dbparams:walk_distance1(i)= AM[(origin,destination[i])]['AM2dis']
		--dbparams:walk_distance2(i)= PM[(destination[i],origin)]['PM2dis']
		--dbparams:central_dummy(i)=ZONE[destination[i]]['central_dummy']
		--origin is home mtz, destination[i] is zone from 1 to 1169
		--0 if origin == destination
		
		central_dummy[i] = dbparams:central_dummy(i)

		cost_taxi_1[i] = 3.4+((d1[i]*(d1[i]>10 and 1 or 0)-10*(d1[i]>10 and 1 or 0))/0.35+(d1[i]*(d1[i]<=10 and 1 or 0)+10*(d1[i]>10 and 1 or 0))/0.4)*0.22+ dbparams:cost_car_ERP_first(i)+central_dummy[i]*3
		cost_taxi_2[i] = 3.4+((d2[i]*(d2[i]>10 and 1 or 0)-10*(d2[i]>10 and 1 or 0))/0.35+(d2[i]*(d2[i]<=10 and 1 or 0)+10*(d2[i]>10 and 1 or 0))/0.4)*0.22+ dbparams:cost_car_ERP_second(i)+central_dummy[i]*3
		cost_taxi[i] = cost_taxi_1[i] + cost_taxi_2[i] + cost_increase
		
		cost_SMS_1[i] = 3.4+((d1[i]*(d1[i]>10 and 1 or 0)-10*(d1[i]>10 and 1 or 0))/0.35+(d1[i]*(d1[i]<=10 and 1 or 0)+10*(d1[i]>10 and 1 or 0))/0.4)*0.22+ dbparams:cost_car_ERP_first(i)+central_dummy[i]*3
		cost_SMS_2[i] = 3.4+((d2[i]*(d2[i]>10 and 1 or 0)-10*(d2[i]>10 and 1 or 0))/0.35+(d2[i]*(d2[i]<=10 and 1 or 0)+10*(d2[i]>10 and 1 or 0))/0.4)*0.22+ dbparams:cost_car_ERP_second(i)+central_dummy[i]*3
		cost_SMS[i] = (cost_SMS_1[i] + cost_SMS_2[i])*0.6 + cost_increase
		
		cost_SMS_Pool_1[i] = 3.4+((d1[i]*(d1[i]>10 and 1 or 0)-10*(d1[i]>10 and 1 or 0))/0.35+(d1[i]*(d1[i]<=10 and 1 or 0)+10*(d1[i]>10 and 1 or 0))/0.4)*0.22+ dbparams:cost_car_ERP_first(i)+central_dummy[i]*3
		cost_SMS_Pool_2[i] = 3.4+((d2[i]*(d2[i]>10 and 1 or 0)-10*(d2[i]>10 and 1 or 0))/0.35+(d2[i]*(d2[i]<=10 and 1 or 0)+10*(d2[i]>10 and 1 or 0))/0.4)*0.22+ dbparams:cost_car_ERP_second(i)+central_dummy[i]*3
		cost_SMS_Pool[i] = (cost_SMS_Pool_1[i] + cost_SMS_Pool_2[i])*0.6*0.7 + cost_increase
		
		cost_AMOD_1[i] = 3.4+((d1[i]*(d1[i]>10 and 1 or 0)-10*(d1[i]>10 and 1 or 0))/0.35+(d1[i]*(d1[i]<=10 and 1 or 0)+10*(d1[i]>10 and 1 or 0))/0.4)*0.22+ dbparams:cost_car_ERP_first(i)+central_dummy[i]*3
		cost_AMOD_2[i] = 3.4+((d2[i]*(d2[i]>10 and 1 or 0)-10*(d2[i]>10 and 1 or 0))/0.35+(d2[i]*(d2[i]<=10 and 1 or 0)+10*(d2[i]>10 and 1 or 0))/0.4)*0.22+ dbparams:cost_car_ERP_second(i)+central_dummy[i]*3
		cost_AMOD[i] = (cost_AMOD_1[i] + cost_AMOD_2[i])*0.6/3 + cost_increase
		
		cost_AMOD_Pool_1[i] = 3.4+((d1[i]*(d1[i]>10 and 1 or 0)-10*(d1[i]>10 and 1 or 0))/0.35+(d1[i]*(d1[i]<=10 and 1 or 0)+10*(d1[i]>10 and 1 or 0))/0.4)*0.22+ dbparams:cost_car_ERP_first(i)+central_dummy[i]*3
		cost_AMOD_Pool_2[i] = 3.4+((d2[i]*(d2[i]>10 and 1 or 0)-10*(d2[i]>10 and 1 or 0))/0.35+(d2[i]*(d2[i]<=10 and 1 or 0)+10*(d2[i]>10 and 1 or 0))/0.4)*0.22+ dbparams:cost_car_ERP_second(i)+central_dummy[i]*3
		cost_AMOD_Pool[i] = (cost_AMOD_Pool_1[i] + cost_AMOD_Pool_2[i])*0.6*0.7/3 + cost_increase
		
		local aed = 2.0 -- Access egress distance
		cost_Rail_SMS_AE_1[i] = 3.4+((aed*(aed>10 and 1 or 0)-10*(aed>10 and 1 or 0))/0.35+(aed*(aed<=10 and 1 or 0)+10*(aed>10 and 1 or 0))/0.4)*0.22+ dbparams:cost_car_ERP_first(i)+central_dummy[i]*3
		cost_Rail_SMS_AE_2[i] = 3.4+((aed*(aed>10 and 1 or 0)-10*(aed>10 and 1 or 0))/0.35+(aed*(aed<=10 and 1 or 0)+10*(aed>10 and 1 or 0))/0.4)*0.22+ dbparams:cost_car_ERP_second(i)+central_dummy[i]*3
		cost_Rail_SMS_AE_avg[i] = (cost_Rail_SMS_AE_1[i] + cost_Rail_SMS_AE_2[i])/2
		cost_Rail_SMS[i] = cost_public_first[i] + cost_public_second[i] + cost_increase + cost_Rail_SMS_AE_avg[i] * 2 * 0.6
	
		cost_Rail_SMS_AE_Pool_1[i] = 3.4+((aed*(aed>10 and 1 or 0)-10*(aed>10 and 1 or 0))/0.35+(aed*(aed<=10 and 1 or 0)+10*(aed>10 and 1 or 0))/0.4)*0.22+ dbparams:cost_car_ERP_first(i) + central_dummy[i]*3
		cost_Rail_SMS_AE_Pool_2[i] = 3.4+((aed*(aed>10 and 1 or 0)-10*(aed>10 and 1 or 0))/0.35+(aed*(aed<=10 and 1 or 0)+10*(aed>10 and 1 or 0))/0.4)*0.22 + dbparams:cost_car_ERP_second(i) + central_dummy[i]*3
		cost_Rail_SMS_AE_Pool_avg[i] = (cost_Rail_SMS_AE_Pool_1[i] + cost_Rail_SMS_AE_Pool_2[i])/2.0
		cost_Rail_SMS_Pool[i] = cost_public_first[i] + cost_public_second[i] + cost_increase + cost_Rail_SMS_AE_avg[i] * 2 * 0.6 *0.7
		
		cost_Rail_AMOD_AE_1[i] = 3.4+((aed*(aed>10 and 1 or 0)-10*(aed>10 and 1 or 0))/0.35+(aed*(aed<=10 and 1 or 0)+10*(aed>10 and 1 or 0))/0.4)*0.22+ dbparams:cost_car_ERP_first(i)+central_dummy[i]*3
		cost_Rail_AMOD_AE_2[i] = 3.4+((aed*(aed>10 and 1 or 0)-10*(aed>10 and 1 or 0))/0.35+(aed*(aed<=10 and 1 or 0)+10*(aed>10 and 1 or 0))/0.4)*0.22+ dbparams:cost_car_ERP_second(i)+central_dummy[i]*3
		cost_Rail_AMOD_AE_avg[i] = (cost_Rail_AMOD_AE_1[i] + cost_Rail_AMOD_AE_2[i])/2
		cost_Rail_AMOD[i] = cost_public_first[i] + cost_public_second[i] + cost_increase + cost_Rail_AMOD_AE_avg[i] * 2 * 0.6/3
	
		cost_Rail_AMOD_AE_Pool_1[i] = 3.4+((aed*(aed>10 and 1 or 0)-10*(aed>10 and 1 or 0))/0.35+(aed*(aed<=10 and 1 or 0)+10*(aed>10 and 1 or 0))/0.4)*0.22+ dbparams:cost_car_ERP_first(i) + central_dummy[i]*3
		cost_Rail_AMOD_AE_Pool_2[i] = 3.4+((aed*(aed>10 and 1 or 0)-10*(aed>10 and 1 or 0))/0.35+(aed*(aed<=10 and 1 or 0)+10*(aed>10 and 1 or 0))/0.4)*0.22 + dbparams:cost_car_ERP_second(i) + central_dummy[i]*3
		cost_Rail_AMOD_AE_Pool_avg[i] = (cost_Rail_AMOD_AE_Pool_1[i] + cost_Rail_AMOD_AE_Pool_2[i])/2.0
		cost_Rail_AMOD_Pool[i] = cost_public_first[i] + cost_public_second[i] + cost_increase + cost_Rail_AMOD_AE_avg[i] * 2 * 0.6 *0.7/3
	
	
		cost_over_income_bus[i]=30*cost_bus[i]/(0.5+income_mid)
		cost_over_income_mrt[i]=30*cost_mrt[i]/(0.5+income_mid)
		cost_over_income_Rail_SMS[i]=30*cost_Rail_SMS[i]/(0.5+income_mid)
		cost_over_income_private_bus[i]=30*cost_private_bus[i]/(0.5+income_mid)
		cost_over_income_drive1[i] = 30 * cost_drive1[i]/(0.5+income_mid)
		cost_over_income_share2[i] = 30 * cost_share2[i]/(0.5+income_mid)
		cost_over_income_share3[i] = 30 * cost_share3[i]/(0.5+income_mid)
		cost_over_income_motor[i]=30*cost_motor[i]/(0.5+income_mid)
		cost_over_income_taxi[i]=30*cost_taxi[i]/(0.5+income_mid)
		cost_over_income_SMS[i]=30*cost_SMS[i]/(0.5+income_mid)
		cost_over_income_SMS_Pool[i]=30*cost_SMS_Pool[i]/(0.5+income_mid)
		cost_over_income_Rail_SMS_Pool[i]=30*cost_Rail_SMS_Pool[i]/(0.5+income_mid)
		cost_over_income_AMOD[i]=30*cost_AMOD[i]/(0.5+income_mid)
		cost_over_income_AMOD_Pool[i]=30*cost_AMOD_Pool[i]/(0.5+income_mid)
		cost_over_income_Rail_AMOD_Pool[i]=30*cost_Rail_AMOD_Pool[i]/(0.5+income_mid)
		cost_over_income_Rail_AMOD[i]=30*cost_Rail_AMOD[i]/(0.5+income_mid)
		
		--dbparams:tt_public_ivt_first(i) = AM[(origin,destination[i])]['pub_ivt']
		--dbparams:tt_public_ivt_second(i) = PM[(destination[i],origin)]['pub_ivt']
		--dbparams:tt_public_out_first(i) = AM[(origin,destination[i])]['pub_out']
		--dbparams:tt_public_out_second(i) = PM[(destination[i],origin)]['pub_out']
		--for the above 4 variables, origin is home, destination[i] is zone from 1 to 1169
		--0 if origin == destination
		tt_public_ivt_first[i] = dbparams:tt_public_ivt_first(i)
		tt_public_ivt_second[i] = dbparams:tt_public_ivt_second(i)
		tt_public_out_first[i] = dbparams:tt_public_out_first(i)
		tt_public_out_second[i] = dbparams:tt_public_out_second(i)
		

		--dbparams:tt_car_ivt_first(i) = AM[(origin,destination[i])]['car_ivt']
		--dbparams:tt_car_ivt_second(i) = PM[(destination[i],origin)]['car_ivt']
		tt_car_ivt_first[i] = dbparams:tt_car_ivt_first(i)
		tt_car_ivt_second[i] = dbparams:tt_car_ivt_second(i)

		tt_bus[i] = tt_public_ivt_first[i]+ tt_public_ivt_second[i]+tt_public_out_first[i]+tt_public_out_second[i]
		tt_mrt[i] = tt_public_ivt_first[i]+ tt_public_ivt_second[i]+tt_public_out_first[i]+tt_public_out_second[i]
		tt_Rail_SMS[i] = tt_public_ivt_first[i]+ tt_public_ivt_second[i]+(tt_public_out_first[i]+tt_public_out_second[i])/6.0
		tt_private_bus[i] = tt_car_ivt_first[i] + tt_car_ivt_second[i]
		tt_drive1[i] = tt_car_ivt_first[i] + tt_car_ivt_second[i] + 1.0/6
		tt_share2[i] = tt_car_ivt_first[i] + tt_car_ivt_second[i] + 1.0/6
		tt_share3[i] = tt_car_ivt_first[i] + tt_car_ivt_second[i] + 1.0/6
		tt_motor[i] = tt_car_ivt_first[i] + tt_car_ivt_second[i] + 1.0/6
		tt_walk[i] = (d1[i]+d2[i])/5
		tt_taxi[i] = tt_car_ivt_first[i] + tt_car_ivt_second[i] + 1.0/6
		tt_SMS[i] = tt_car_ivt_first[i] + tt_car_ivt_second[i] + 1.0/6
		tt_SMS_Pool[i] = tt_car_ivt_first[i] + tt_car_ivt_second[i]+1.0/6 + 1/10+(d1[i]+d2[i])/2/60 + 1.0/6
		tt_Rail_SMS_Pool[i] = tt_public_ivt_first[i]+ tt_public_ivt_second[i]+(tt_public_out_first[i]+tt_public_out_second[i])/6 +(aed+aed)/60+1/10
		tt_AMOD[i] = tt_car_ivt_first[i] + tt_car_ivt_second[i] + 1.0/6
		tt_AMOD_Pool[i] = tt_car_ivt_first[i] + tt_car_ivt_second[i]+1.0/6 + 1/10+(d1[i]+d2[i])/2/60 + 1.0/6
		tt_Rail_AMOD_Pool[i] = tt_public_ivt_first[i]+ tt_public_ivt_second[i]+(tt_public_out_first[i]+tt_public_out_second[i])/6 +(aed+aed)/60+1/10
		tt_Rail_AMOD[i] = tt_public_ivt_first[i]+ tt_public_ivt_second[i]+(tt_public_out_first[i]+tt_public_out_second[i])/6.0

		--dbparams:average_transfer_number(i) = (AM[(origin,destination[i])]['avg_transfer'] + PM[(destination[i],origin)]['avg_transfer'])/2
		--origin is home, destination[i] is zone from 1 to 1169
		-- 0 if origin == destination
		average_transfer_number[i] = dbparams:average_transfer_number(i)

		--dbparams:employment(i) = ZONE[i]['employment']
		--dbparams:population(i) = ZONE[i]['population']
		--dbparams:area(i) = ZONE[i]['area']
		--dbparams:shop(i) = ZONE[i]['shop']
		employment[i] = dbparams:employment(i)
		population[i] = dbparams:population(i)
		area[i] = dbparams:area(i)
		shop[i] = dbparams:shop(i)
	end

	local V_counter = 0
	print ("Iside compute utilitiers",543) 
	--utility function for bus 1-1169
	for i =1,1169 do
		V_counter = V_counter + 1
		utility[V_counter] = beta_cons_bus + cost_over_income_bus[i] * (1- missing_income) * beta_cost_bus_mrt_1 + cost_bus[i] * missing_income * beta_cost_bus_mrt_2 + tt_bus[i] * beta_tt_bus_mrt + beta_central_bus_mrt * central_dummy[i] + beta_log * log(employment[i]+math.exp(beta_area)*area[i]+(beta_population)*population[i]) + (d1[i]+d2[i]) * beta_distance_bus_mrt + bundled_variables.beta_female_bus * female_dummy + beta_zero_bus* zero_car + beta_oneplus_bus* one_plus_car+ beta_twoplus_bus* two_plus_car
	end

	--utility function for mrt 1-1169
	for i=1,1169 do
		V_counter = V_counter +1
		utility[V_counter] = beta_cons_mrt + cost_over_income_mrt[i] * (1- missing_income) * beta_cost_bus_mrt_1 + cost_mrt[i] * missing_income * beta_cost_bus_mrt_2 + tt_mrt[i] * beta_tt_bus_mrt + beta_central_bus_mrt * central_dummy[i] + beta_log * math.log(employment[i]+math.exp(beta_area)*area[i]+(beta_population)*population[i]) + (d1[i]+d2[i]) * beta_distance_bus_mrt + bundled_variables.beta_female_mrt * female_dummy+ beta_zero_mrt*zero_car+ beta_oneplus_mrt*one_plus_car+beta_twoplus_mrt*two_plus_car
	end


	--utility function for drive1 1-1169
	for i=1,1169 do
		V_counter = V_counter +1
		utility[V_counter] = beta_cons_drive1 + cost_over_income_drive1[i] * (1 - missing_income) * beta_cost_drive1_1 + cost_drive1[i] * missing_income * beta_cost_drive1_2 + tt_drive1[i] * beta_tt_drive1 + beta_central_drive1 * central_dummy[i] + beta_log * math.log(employment[i]+math.exp(beta_area)*area[i]+(beta_population)*population[i] + 1) + (d1[i]+d2[i]) * beta_distance_drive1 + beta_zero_drive1 *zero_car + beta_oneplus_drive1 * one_plus_car + beta_twoplus_drive1 * two_plus_car + beta_threeplus_drive1 * three_plus_car + bundled_variables.beta_female_drive1 * female_dummy
	end

	--utility function for share2 1-1169
	for i=1,1169 do
		V_counter = V_counter +1
		utility[V_counter] = beta_cons_share2 + cost_over_income_share2[i] * (1 - missing_income) * beta_cost_share2_1 + cost_share2[i] * missing_income * beta_cost_share2_2 + tt_share2[i] * beta_tt_drive1 + beta_central_share2 * central_dummy[i] + beta_log * math.log(employment[i]+math.exp(beta_area)*area[i]+(beta_population)*population[i] + 1) + (d1[i]+d2[i]) * beta_distance_share2 + beta_zero_share2 *zero_car + beta_oneplus_share2 * one_plus_car + beta_twoplus_share2 * two_plus_car + beta_threeplus_share2 * three_plus_car + bundled_variables.beta_female_share2 * female_dummy
	end

	--utility function for share3 1-1169
	for i=1,1169 do
		V_counter = V_counter +1
		utility[V_counter] = beta_cons_share3 + cost_over_income_share3[i] * (1 - missing_income) * beta_cost_share3_1 + cost_share3[i] * missing_income * beta_cost_share3_2 + tt_share3[i] * beta_tt_drive1 + beta_central_share3 * central_dummy[i] + beta_log * math.log(employment[i]+math.exp(beta_area)*area[i]+(beta_population)*population[i] + 1) + (d1[i]+d2[i]) * beta_distance_share3 + beta_zero_share3 *zero_car + beta_oneplus_share3 * one_plus_car + beta_twoplus_share3 * two_plus_car + beta_threeplus_share3 * three_plus_car + bundled_variables.beta_female_share3 * female_dummy
	end

	--utility function for motor 1-1169
	for i=1,1169 do
		V_counter = V_counter +1
		utility[V_counter] = beta_cons_motor + cost_over_income_motor[i] * (1 - missing_income) * beta_cost_motor_1 + cost_motor[i] * missing_income * beta_cost_motor_2 + tt_motor[i] * beta_tt_drive1 + beta_central_motor * central_dummy[i] + beta_log * math.log(employment[i]+math.exp(beta_area)*area[i]+(beta_population)*population[i] + 1) + (d1[i]+d2[i]) * beta_distance_motor + bundled_variables.beta_zero_motor *zero_motor + bundled_variables.beta_oneplus_motor * one_plus_motor + bundled_variables.beta_twoplus_motor * two_plus_motor + bundled_variables.beta_threeplus_motor * three_plus_motor + bundled_variables.beta_female_motor * female_dummy + beta_zero_car_motor*zero_car+beta_oneplus_car_motor*one_plus_car+ beta_twoplus_car_motor*two_plus_car
	end

	--utility function for walk 1-1169
	for i=1,1169 do
		V_counter = V_counter +1
		utility[V_counter] = beta_cons_walk + tt_walk[i] * beta_tt_walk + beta_central_walk * central_dummy[i] + beta_log * math.log(employment[i]+math.exp(beta_area)*area[i]+(beta_population)*population[i] + 1) + (d1[i]+d2[i]) * beta_distance_walk + bundled_variables.beta_female_walk * female_dummy + beta_zero_walk*zero_car + beta_oneplus_walk*one_plus_car+beta_twoplus_walk*two_plus_car
	end

	--utility function for taxi 1-1169
	for i=1,1169 do
		V_counter = V_counter +1
		utility[V_counter] = beta_cons_taxi + cost_over_income_taxi[i] * (1-missing_income)* beta_cost_taxi_1 + cost_taxi[i]*missing_income * beta_cost_taxi_2 + tt_taxi[i] * beta_tt_taxi + beta_central_taxi * central_dummy[i] + beta_log * math.log(employment[i]+math.exp(beta_area)*area[i]+(beta_population)*population[i] + 1) + (d1[i]+d2[i]) * beta_distance_taxi + bundled_variables.beta_female_taxi * female_dummy + beta_zero_taxi*zero_car+beta_oneplus_taxi*one_plus_car+beta_twoplus_taxi*two_plus_car
	end

	--utility function for SMS 1-1169
	for i=1,1169 do
		V_counter = V_counter +1
		utility[V_counter] = beta_cons_SMS + cost_over_income_SMS[i] * (1-missing_income)* beta_cost_SMS_1 + cost_SMS[i] * missing_income * beta_cost_SMS_2 + tt_SMS[i] * beta_tt_SMS + beta_central_SMS * central_dummy[i] + beta_log * math.log(exp(beta_area)*area[i]+math.exp(beta_population)*population[i]) + (d1[i]+d2[i]) * beta_distance_SMS + bundled_variables.beta_female_SMS * female_dummy + beta_zero_SMS*zero_car+beta_oneplus_SMS*one_plus_car+beta_twoplus_SMS*two_plus_car
	end

	--utility function for Rail_SMS 1-1169
	for i=1,1169 do
		V_counter = V_counter +1
		utility[V_counter] = beta_cons_Rail_SMS + cost_over_income_Rail_SMS[i] * (1- missing_income) * beta_cost_Rail_SMS_1 + cost_Rail_SMS[i] * missing_income * beta_cost_Rail_SMS_2 + tt_Rail_SMS[i] * beta_tt_Rail_SMS + beta_central_Rail_SMS * central_dummy[i] + beta_log * math.log(exp(beta_area)*area[i]+exp(beta_population)*population[i]) + (d1[i]+d2[i]) * beta_distance_Rail_SMS + bundled_variables.beta_female_Rail_SMS * female_dummy + beta_zero_Rail_SMS*zero_car+ beta_oneplus_Rail_SMS*one_plus_car+beta_twoplus_Rail_SMS*two_plus_car
	end
	
			--utility function for SMS_Pool 1-1169
	for i=1,1169 do
		V_counter = V_counter +1
		utility[V_counter] = beta_cons_SMS_Pool + cost_over_income_SMS_Pool[i] * (-missing_income)* beta_cost_SMS_Pool_1 + cost_SMS_Pool[i]* beta_cost_bus_mrt_2 + tt_SMS_Pool[i] * beta_tt_SMS_Pool + beta_central_SMS_Pool * central_dummy[i] + beta_log * math.log(math.exp(beta_area)*area[i]+math.exp(beta_population)*population[i]) + (d1[i]+d2[i]) * beta_distance_SMS_Pool + bundled_variables.beta_female_SMS_Pool * female_dummy + beta_zero_SMS_Pool*zero_car+beta_oneplus_SMS_Pool*one_plus_car+beta_twoplus_SMS_Pool*two_plus_car
	end
	
	--utility function for Rail_SMS_Pool 1-1169
	for i=1,1169 do
		V_counter = V_counter +1
		utility[V_counter] = beta_cons_Rail_SMS_Pool + cost_over_income_Rail_SMS_Pool[i] * (1- missing_income) * beta_cost_Rail_SMS_Pool_1 + cost_Rail_SMS_Pool[i] * beta_cost_Rail_SMS_Pool_2 + tt_Rail_SMS_Pool[i] * beta_tt_Rail_SMS_Pool + beta_central_Rail_SMS_Pool * central_dummy[i] + beta_log * math.log(math.exp(beta_area)*area[i]+math.exp(beta_population)*population[i]) + (d1[i]+d2[i]) * beta_distance_Rail_SMS_Pool + bundled_variables.beta_female_Rail_SMS_Pool * female_dummy + beta_zero_Rail_SMS_Pool*zero_car+ beta_oneplus_Rail_SMS_Pool*one_plus_car+beta_twoplus_Rail_SMS_Pool*two_plus_car
	end
	
		--utility function for AMOD 1-1169
	for i=1,1169 do
		V_counter = V_counter +1
		utility[V_counter] = 0--beta_cons_AMOD + cost_over_income_AMOD[i] * (1-missing_income)* beta_cost_AMOD_1 + cost_AMOD[i] * missing_income * beta_cost_AMOD_2 + tt_SMS[i] * beta_tt_AMOD + beta_central_AMOD * central_dummy[i] + beta_log * math.log(exp(beta_area)*area[i]+math.exp(beta_population)*population[i]) + (d1[i]+d2[i]) * beta_distance_AMOD + bundled_variables.beta_female_AMOD * female_dummy + beta_zero_AMOD*zero_car+beta_oneplus_AMOD*one_plus_car+beta_twoplus_AMOD*two_plus_car
	end

	--utility function for Rail_AMOD 1-1169
	for i=1,1169 do
		V_counter = V_counter +1
		utility[V_counter] = 0--beta_cons_Rail_AMOD + cost_over_income_Rail_AMOD[i] * (1- missing_income) * beta_cost_Rail_AMOD_1 + cost_Rail_AMOD[i] * missing_income * beta_cost_Rail_AMOD_2 + tt_Rail_AMOD[i] * beta_tt_Rail_AMOD + beta_central_Rail_AMOD * central_dummy[i] + beta_log * math.log(exp(beta_area)*area[i]+exp(beta_population)*population[i]) + (d1[i]+d2[i]) * beta_distance_Rail_AMOD + bundled_variables.beta_female_Rail_AMOD * female_dummy + bundled_variables.beta_zero_Rail_AMOD*zero_car+ bundled_variables.beta_oneplus_Rail_AMOD*one_plus_car+bundled_variables.beta_twoplus_Rail_AMOD*two_plus_car
	end
	
			--utility function for AMOD_Pool 1-1169
	for i=1,1169 do
		V_counter = V_counter +1
		utility[V_counter] = 0--beta_cons_AMOD_Pool + cost_over_income_AMOD_Pool[i] * (-missing_income)* beta_cost_AMOD_Pool_1 + cost_AMOD_Pool[i]* beta_cost_bus_mrt_2 + tt_AMOD_Pool[i] * beta_tt_AMOD_Pool + beta_central_AMOD_Pool * central_dummy[i] + beta_log * math.log(math.exp(beta_area)*area[i]+math.exp(beta_population)*population[i]) + (d1[i]+d2[i]) * beta_distance_AMOD_Pool + bundled_variables.beta_female_AMOD_Pool * female_dummy + bundled_variables.beta_zero_AMOD_Pool*zero_car+bundled_variables.beta_oneplus_AMOD_Pool*one_plus_car+bundled_variables.beta_twoplus_AMOD_Pool*two_plus_car
	end
	
	--utility function for Rail_AMOD_Pool 1-1169
	for i=1,1169 do
		V_counter = V_counter +1
		utility[V_counter] = 0--beta_cons_Rail_AMOD_Pool + cost_over_income_Rail_AMOD_Pool[i] * (1- missing_income) * beta_cost_Rail_AMOD_Pool_1 + cost_Rail_AMOD_Pool[i] * beta_cost_Rail_AMOD_Pool_2 + tt_Rail_AMOD_Pool[i] * beta_tt_Rail_AMOD_Pool + beta_central_Rail_AMOD_Pool * central_dummy[i] + beta_log * math.log(math.exp(beta_area)*area[i]+math.exp(beta_population)*population[i]) + (d1[i]+d2[i]) * beta_distance_Rail_AMOD_Pool + bundled_variables.beta_female_Rail_AMOD_Pool * female_dummy + bundled_variables.beta_zero_Rail_AMOD_Pool*zero_car+ bundled_variables.beta_oneplus_Rail_AMOD_Pool*one_plus_car+bundled_variables.beta_twoplus_Rail_AMOD_Pool*two_plus_car
	end
	
end


--availability
--the logic to determine availability is the same with current implementation
local availability = {}
local function computeAvailabilities(params,dbparams)

	for i = 1, 1169*16 do 

		availability[i] = dbparams:availability(i)
	end
end

--scale
local scale = 1 --for all choices

-- function to call from C++ preday simulator
-- params and dbparams tables contain data passed from C++
-- to check variable bindings in params or dbparams, refer PredayLuaModel::mapClasses() function in dev/Basic/medium/behavioral/lua/PredayLuaModel.cpp
function choose_tmdw(params,dbparams)
	print ("Reached choose_tmdw")
	computeUtilities(params,dbparams) 
	computeAvailabilities(params,dbparams)
	local probability = calculate_probability("mnl", choice, utility, availability, scale)
	return make_final_choice(probability)
end

-- function to call from C++ preday simulator for logsums computation
-- params and dbparams tables contain data passed from C++
-- to check variable bindings in params or dbparams, refer PredayLuaModel::mapClasses() function in dev/Basic/medium/behavioral/lua/PredayLuaModel.cpp
function compute_logsum_tmdw(params,dbparams)
	computeUtilities(params,dbparams) 
	computeAvailabilities(params,dbparams)
	return compute_mnl_logsum(utility, availability)
end
