package sim_mob.vis.controls;

import java.awt.*;

import java.awt.image.BufferedImage;
import java.util.*;

import sim_mob.vis.network.basic.*;
import sim_mob.vis.network.*;
import sim_mob.vis.simultion.*;


/**
 * Represents an actual visualizer for the network. Handles scaling, etc. 
 * TODO: When zoomed in, it still draws the entire map. May want to "tile" drawn sections.
 */
public class NetworkVisualizer {
	private RoadNetwork network;
	private SimulationResults simRes;
	private int currFrameTick;
	private BufferedImage buffer;
	private int width100Percent;
	private int height100Percent;
	double currPercentZoom;
	private static final double  ZOOM_IN_CRITICAL = 1.6;
	
	public int getCurrFrameTick() { return currFrameTick; }
	public boolean incrementCurrFrameTick(int amt) {
		return setCurrFrameTick(currFrameTick+amt);
	}
	public boolean setCurrFrameTick(int newVal) {
		if (newVal<0 || simRes==null || newVal>=simRes.ticks.size()) {
			return false;
		}
		currFrameTick = newVal;
		return true;
	}
	
	//For clicking
	private static final double NEAR_THRESHHOLD = 20;
	
	
	public NetworkVisualizer() {
	}
	
	public BufferedImage getImage() {
		return buffer;
	}
	
	
	//Retrieve the node at the given screen position, or null if there's none.
	public Node getNodeAt(Point screenPos) {
		//First, convert the screen coordinates to centimeters
		//DPoint realPos = new DPoint();  //Note: Might be needed later; for now, we can just use ScaledPoints
		for (Node n : network.getNodes().values()) {
			if (screenPos.distance(n.getPos().getX(), n.getPos().getY()) <= NEAR_THRESHHOLD) {
				return n;
			}
		}
		return null;
	}
	
	
	public void setSource(RoadNetwork network, SimulationResults simRes, double initialZoom, int width100Percent, int height100Percent) {
		//Save
		this.network = network;
		this.simRes = simRes;
		this.currFrameTick = 0;
		this.width100Percent = width100Percent;
		this.height100Percent = height100Percent;
		
		//Recalc
		redrawAtScale(initialZoom);
	}
	
	//Negative numbers mean zoom out that many times.
	public void zoomIn(int number) {
		//Each tick increases zoom by 10%
		redrawAtScale(currPercentZoom + currPercentZoom*number*0.10);
		
		//System.out.println("zoom number: "+number);
		//System.out.println("current percent zoom: " + currPercentZoom);
	}
	
	public void redrawAtScale(double percent) {
		//Save
		currPercentZoom = percent;
		
		//System.out.println("percent: "+percent);

		
		//Determine the width and height of our canvas.
		int width = (int)(width100Percent * percent);
		int height = (int)(height100Percent * percent);
		buffer = new BufferedImage(width, height, BufferedImage.TYPE_INT_RGB);
		
		//Make sure our canvas is always slightly bigger than the original size...
		double width5Percent = 0.05 * (network.getLowerRight().x - network.getTopLeft().x);
		double height5Percent = 0.05 * (network.getLowerRight().y - network.getTopLeft().y);
		DPoint newTL = new DPoint(network.getTopLeft().x-width5Percent, network.getTopLeft().y-height5Percent);
		DPoint newLR = new DPoint(network.getLowerRight().x+width5Percent, network.getLowerRight().y+height5Percent);
		
		
		
		//Scale all points
		ScaledPointGroup.SetNewScaleContext(new ScaleContext(percent, newTL, newLR, width, height));
		//ScaledPoint.ScaleAllPoints(percent, newTL, newLR, width, height);
		redrawAtCurrScale();
	}
	
	public void redrawAtCurrScale() {
		//System.out.println(" refresh");
		
		//Retrieve a graphics object; ensure it'll anti-alias
		Graphics2D g = (Graphics2D)buffer.getGraphics();
		g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
		
		//Fill the background
		g.setBackground(Color.WHITE);
		g.clearRect(0, 0, buffer.getWidth(), buffer.getHeight());
		
		//Draw nodes
		for (Node n : network.getNodes().values()) {
			if(currPercentZoom<=ZOOM_IN_CRITICAL){
				n.draw(g);
			}
			else{
				if(n.getIsUni()){
					continue;
				}
				else{
					n.draw(g);
				}		
			}
		}
		
		if(currPercentZoom <= ZOOM_IN_CRITICAL){
			//Draw segments
			for (Segment sn : network.getSegments().values()) {
				sn.draw(g);
			}
		}
		//Draw links
		for (Link ln : network.getLinks().values()) {
			ln.draw(g);
		}
		
		//Names go on last; make sure we don't draw them twice...
		Set<String> alreadyDrawn = new HashSet<String>();
		for (Link ln : network.getLinks().values()) {
			
			String key1 = ln.getName() + ln.getStart().toString() + ":" + ln.getEnd().toString();
			String key2 = ln.getName() + ln.getEnd().toString() + ":" + ln.getStart().toString();
			
			if (alreadyDrawn.contains(key1) || alreadyDrawn.contains(key2)) {
				continue;
			}
			alreadyDrawn.add(key1);
			ln.drawName(g);
		}
		
		//Draw out lanes only it is zoom to certain scale
		if(currPercentZoom>ZOOM_IN_CRITICAL){
			//Draw Lanes
			for (Hashtable<Integer,LaneMarking> lineMarkingTable : network.getLaneMarkings().values()) {
				for(LaneMarking lineMarking : lineMarkingTable.values()){
					lineMarking.draw(g);
				}
			}
			
			//Draw Perdestrain Crossing
			for(Crossing crossing : network.getCrossings().values()){
				crossing.draw(g);
			}
		
			//Now draw out signal
			for(SignalLineTick at: simRes.ticks.get(currFrameTick).signalLineTicks.values()){
						
				//Get Intersection ID
				Intersection tempIntersection = network.getIntersection().get(at.getIntersectionID());
				//Get Light color
				ArrayList<ArrayList<Integer>> allVehicleLights =  at.getVehicleLights();
				ArrayList<Integer> allPedestrainLights = at.getPedestrianLights();
				
				//Light Colors
				ArrayList<Integer> vaLights = allVehicleLights.get(0);
				ArrayList<Integer> vbLights = allVehicleLights.get(1);
				ArrayList<Integer> vcLights = allVehicleLights.get(2);
				ArrayList<Integer> vdLights = allVehicleLights.get(3);

				//Light Lines
				ArrayList<ArrayList<TrafficSignalLine>> vaSignalLine = tempIntersection.getVaTrafficSignal();
				ArrayList<ArrayList<TrafficSignalLine>> vbSignalLine = tempIntersection.getVbTrafficSignal();
				ArrayList<ArrayList<TrafficSignalLine>> vcSignalLine = tempIntersection.getVcTrafficSignal();
				ArrayList<ArrayList<TrafficSignalLine>> vdSignalLine = tempIntersection.getVdTrafficSignal();
				
				ArrayList<Crossing> tempCrossings = tempIntersection.getSignalCrossings();
				
				//DrawLights
				drawTrafficLines(g,vaSignalLine, vaLights);
				drawTrafficLines(g,vbSignalLine, vbLights);
				drawTrafficLines(g,vcSignalLine, vcLights);
				drawTrafficLines(g,vdSignalLine, vdLights);
				
				//Draw pedestrian crossing signal
				drawTrafficPedestrainCross(g,tempCrossings,allPedestrainLights);
				
			}

		
		
		}
				
		//Now draw simulation data: cars, etc.
		for (AgentTick at : simRes.ticks.get(currFrameTick).agentTicks.values()) {	
			at.draw(g,currPercentZoom);
		}
		
	}
	
	private void drawTrafficLines(Graphics2D g,ArrayList<ArrayList<TrafficSignalLine>> signalLine, ArrayList<Integer> lightColors){
		
		for(int i = 0; i<signalLine.size();i++){
			
			//Left turn light
			ArrayList<TrafficSignalLine> leftTurnLight = signalLine.get(0);
			leftTurnLight.get(0).drawPerLight(g, lightColors.get(0));	
			
			
			//Straight turn light
			ArrayList<TrafficSignalLine> straightTurnLight = signalLine.get(1);				
			straightTurnLight.get(0).drawPerLight(g, lightColors.get(1));	
			
			
			//Right turn light
			ArrayList<TrafficSignalLine> rightTurnLight = signalLine.get(2);
			rightTurnLight.get(0).drawPerLight(g, lightColors.get(2));	
			
			
		}
				
	}
	
	private void drawTrafficPedestrainCross(Graphics2D g,ArrayList<Crossing> signalPedestrainCrossing, ArrayList<Integer> lightColor){
		

		if(signalPedestrainCrossing.size() != 0 && lightColor.size() != 0
				 && signalPedestrainCrossing.size() == lightColor.size())
		{
			for(int i = 0; i<signalPedestrainCrossing.size();i++)
			{
				//Draw crossing signal
				signalPedestrainCrossing.get(i).drawSignalCrossing(g, lightColor.get(i));
			}
		
		}
		else{
			System.out.println("Error, the signal and crossing are not corresponding to each other -- NetWorkVisualizer, drawTrafficPedestrainCross()");		
		}
		
	}

}

