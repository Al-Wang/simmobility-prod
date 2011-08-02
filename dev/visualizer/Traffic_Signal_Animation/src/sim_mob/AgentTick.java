package sim_mob;

//Simple class for visualizing an agent's state in one time tick.
public class AgentTick {
	public int agentID;
	
	public double agentX;
	public double agentY;

	public int agentScaledX;
	public int agentScaledY;
	
	public double carDir;
	
	//Do these properties belong here? 
	public int phaseSignal;
	public double ds;
	public double cycleLen;
	public int phaseCount;
}
