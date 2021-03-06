#include "v_repExtBubbleRob.h"
#include "scriptFunctionData.h"
#include <iostream>
using namespace std;
#include "v_repLib.h"

#ifdef _WIN32
    #ifdef QT_COMPIL
        #include <direct.h>
    #else
        #include <shlwapi.h>
        #pragma comment(lib, "Shlwapi.lib")
    #endif
#endif /* _WIN32 */
#if defined (__linux) || defined (__APPLE__)
    #include <unistd.h>
    #define WIN_AFX_MANAGE_STATE
#endif /* __linux || __APPLE__ */

#define CONCAT(x,y,z) x y z
#define strConCat(x,y,z)    CONCAT(x,y,z)

#define PLUGIN_NAME "BubbleRob"

LIBRARY vrepLib;

struct sBubbleRob
{
    int handle;
    int motorHandles[2];
    int sensorHandle;
    float backRelativeVelocities[2];
    int visionsensor[3];
    float duration;
    float backMovementDuration;
    char* waitUntilZero;
};

std::vector<sBubbleRob> allBubbleRobs;
int nextBubbleRobHandle=0;

int getBubbleRobIndexFromHandle(int bubbleRobHandle)
{
    for (unsigned int i=0;i<allBubbleRobs.size();i++)
    {
        if (allBubbleRobs[i].handle==bubbleRobHandle)
            return(i);
    }
    return(-1);
}

// --------------------------------------------------------------------------------------
// simExtBubble_create
// --------------------------------------------------------------------------------------
#define LUA_CREATE_COMMAND "simBubble.create"

const int inArgs_CREATE[]={
    4,
    sim_script_arg_int32|sim_script_arg_table,2,
    sim_script_arg_int32,0,
    sim_script_arg_float|sim_script_arg_table,2,
    sim_script_arg_int32|sim_script_arg_table,3,
};

void LUA_CREATE_CALLBACK(SScriptCallBack* cb)
{
    CScriptFunctionData D;
    int handle=-1;
    if (D.readDataFromStack(cb->stackID,inArgs_CREATE,inArgs_CREATE[0],LUA_CREATE_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        sBubbleRob bubbleRob;
        handle=nextBubbleRobHandle++;
        bubbleRob.handle=handle;
        bubbleRob.motorHandles[0]=inData->at(0).int32Data[0];
        bubbleRob.motorHandles[1]=inData->at(0).int32Data[1];
        bubbleRob.sensorHandle=inData->at(1).int32Data[0];
        bubbleRob.backRelativeVelocities[0]=inData->at(2).floatData[0];
        bubbleRob.backRelativeVelocities[1]=inData->at(2).floatData[1];
        bubbleRob.visionsensor[0]=inData->at(3).int32Data[0];
        bubbleRob.visionsensor[1]=inData->at(3).int32Data[1];
        bubbleRob.visionsensor[2]=inData->at(3).int32Data[2];
        bubbleRob.waitUntilZero=NULL;
        bubbleRob.duration=0.0f;
        allBubbleRobs.push_back(bubbleRob);
    }
    D.pushOutData(CScriptFunctionDataItem(handle));
    D.writeDataToStack(cb->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simExtBubble_destroy
// --------------------------------------------------------------------------------------
#define LUA_DESTROY_COMMAND "simBubble.destroy"

const int inArgs_DESTROY[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_DESTROY_CALLBACK(SScriptCallBack* cb)
{
    CScriptFunctionData D;
    bool success=false;
    if (D.readDataFromStack(cb->stackID,inArgs_DESTROY,inArgs_DESTROY[0],LUA_DESTROY_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=inData->at(0).int32Data[0];
        int index=getBubbleRobIndexFromHandle(handle);
        if (index>=0)
        {
            if (allBubbleRobs[index].waitUntilZero!=NULL)
                allBubbleRobs[index].waitUntilZero[0]=0; // free the blocked thread
            allBubbleRobs.erase(allBubbleRobs.begin()+index);
            success=true;
        }
        else
            simSetLastError(LUA_DESTROY_COMMAND,"Invalid BubbleRob handle.");
    }
    D.pushOutData(CScriptFunctionDataItem(success));
    D.writeDataToStack(cb->stackID);
}
// --------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------
// simExtBubble_start
// --------------------------------------------------------------------------------------
#define LUA_START_COMMAND "simBubble.start"

const int inArgs_START[]={
    3,
    sim_script_arg_int32,0,
    sim_script_arg_float,0,
    sim_script_arg_bool,0,
};

void LUA_START_CALLBACK(SScriptCallBack* cb)
{
    CScriptFunctionData D;
    bool success=false;
    if (D.readDataFromStack(cb->stackID,inArgs_START,inArgs_START[0]-1,LUA_START_COMMAND)) // -1 because the last argument is optional
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=inData->at(0).int32Data[0];
        float duration=inData->at(1).floatData[0];
        bool leaveDirectly=false;
        if (inData->size()>2)
            leaveDirectly=inData->at(2).boolData[0];
        int index=getBubbleRobIndexFromHandle(handle);
        if (index!=-1)
        {
            if (duration<=0.0f)
                leaveDirectly=true;
            allBubbleRobs[index].backMovementDuration=0.0f;
            allBubbleRobs[index].duration=duration;
            if (!leaveDirectly)
                cb->waitUntilZero=1; // the effect of this is that when we leave the callback, the Lua script gets control
                                    // back only when this value turns zero. This allows for "blocking" functions
            success=true;
        }
        else
            simSetLastError(LUA_START_COMMAND,"Invalid BubbleRob handle.");
    }
    D.pushOutData(CScriptFunctionDataItem(success));
    D.writeDataToStack(cb->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simExtBubble_stop
// --------------------------------------------------------------------------------------
#define LUA_STOP_COMMAND "simBubble.stop"

const int inArgs_STOP[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_STOP_CALLBACK(SScriptCallBack* cb)
{
    CScriptFunctionData D;
    bool success=false;
    if (D.readDataFromStack(cb->stackID,inArgs_STOP,inArgs_STOP[0],LUA_STOP_COMMAND))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=inData->at(0).int32Data[0];
        int index=getBubbleRobIndexFromHandle(handle);
        if (index!=-1)
        {
            if (allBubbleRobs[index].waitUntilZero!=NULL)
            {
                allBubbleRobs[index].waitUntilZero[0]=0; // free the blocked thread
                allBubbleRobs[index].waitUntilZero=NULL;
            }
            allBubbleRobs[index].duration=0.0f;
            success=true;
        }
        else
            simSetLastError(LUA_STOP_COMMAND,"Invalid BubbleRob handle.");
    }
    D.pushOutData(CScriptFunctionDataItem(success));
    D.writeDataToStack(cb->stackID);
}
// --------------------------------------------------------------------------------------


VREP_DLLEXPORT unsigned char v_repStart(void* reservedPointer,int reservedInt)
{ // This is called just once, at the start of V-REP.
    // Dynamically load and bind V-REP functions:
    char curDirAndFile[1024];
#ifdef _WIN32
    #ifdef QT_COMPIL
        _getcwd(curDirAndFile, sizeof(curDirAndFile));
    #else
        GetModuleFileName(NULL,curDirAndFile,1023);
        PathRemoveFileSpec(curDirAndFile);
    #endif
#elif defined (__linux) || defined (__APPLE__)
    getcwd(curDirAndFile, sizeof(curDirAndFile));
#endif

    std::string currentDirAndPath(curDirAndFile);
    std::string temp(currentDirAndPath);

#ifdef _WIN32
    temp+="\\v_rep.dll";
#elif defined (__linux)
    temp+="/libv_rep.so";
#elif defined (__APPLE__)
    temp+="/libv_rep.dylib";
#endif /* __linux || __APPLE__ */

    vrepLib=loadVrepLibrary(temp.c_str());
    if (vrepLib==NULL)
    {
        std::cout << "Error, could not find or correctly load v_rep.dll. Cannot start 'BubbleRob' plugin.\n";
        return(0); // Means error, V-REP will unload this plugin
    }
    if (getVrepProcAddresses(vrepLib)==0)
    {
        std::cout << "Error, could not find all required functions in v_rep.dll. Cannot start 'BubbleRob' plugin.\n";
        unloadVrepLibrary(vrepLib);
        return(0); // Means error, V-REP will unload this plugin
    }

    // Check the V-REP version:
    int vrepVer,vrepRev;
    simGetIntegerParameter(sim_intparam_program_version,&vrepVer);
    simGetIntegerParameter(sim_intparam_program_revision,&vrepRev);
    if( (vrepVer<30400) || ((vrepVer==30400)&&(vrepRev<9)) )
    {
        std::cout << "Sorry, your V-REP copy is somewhat old, V-REP 3.4.0 rev9 or higher is required. Cannot start 'BubbleRob' plugin.\n";
        unloadVrepLibrary(vrepLib);
        return(0); // Means error, V-REP will unload this plugin
    }

    simRegisterScriptVariable("simBubble","require('simExtBubbleRob')",0);

    // Register the new functions:
    simRegisterScriptCallbackFunction(strConCat(LUA_CREATE_COMMAND,"@",PLUGIN_NAME),strConCat("number bubbleRobHandle=",LUA_CREATE_COMMAND,"(table_2 motorJointHandles,number sensorHandle,table_2 backRelativeVelocities)"),LUA_CREATE_CALLBACK);
    simRegisterScriptCallbackFunction(strConCat(LUA_DESTROY_COMMAND,"@",PLUGIN_NAME),strConCat("boolean result=",LUA_DESTROY_COMMAND,"(number bubbleRobHandle)"),LUA_DESTROY_CALLBACK);
    simRegisterScriptCallbackFunction(strConCat(LUA_START_COMMAND,"@",PLUGIN_NAME),strConCat("boolean result=",LUA_START_COMMAND,"(number bubbleRobHandle,number duration,boolean returnDirectly=false)"),LUA_START_CALLBACK);
    simRegisterScriptCallbackFunction(strConCat(LUA_STOP_COMMAND,"@",PLUGIN_NAME),strConCat("boolean result=",LUA_STOP_COMMAND,"(number bubbleRobHandle)"),LUA_STOP_CALLBACK);

    // Following for backward compatibility:
    simRegisterScriptVariable("simExtBubble_create",LUA_CREATE_COMMAND,-1);
    simRegisterScriptCallbackFunction(strConCat("simExtBubble_create","@",PLUGIN_NAME),strConCat("Please use the ",LUA_CREATE_COMMAND," notation instead"),0);
    simRegisterScriptVariable("simExtBubble_destroy",LUA_DESTROY_COMMAND,-1);
    simRegisterScriptCallbackFunction(strConCat("simExtBubble_destroy","@",PLUGIN_NAME),strConCat("Please use the ",LUA_DESTROY_COMMAND," notation instead"),0);
    simRegisterScriptVariable("simExtBubble_start",LUA_START_COMMAND,-1);
    simRegisterScriptCallbackFunction(strConCat("simExtBubble_start","@",PLUGIN_NAME),strConCat("Please use the ",LUA_START_COMMAND," notation instead"),0);
    simRegisterScriptVariable("simExtBubble_stop",LUA_STOP_COMMAND,-1);
    simRegisterScriptCallbackFunction(strConCat("simExtBubble_stop","@",PLUGIN_NAME),strConCat("Please use the ",LUA_STOP_COMMAND," notation instead"),0);

    return(9); // initialization went fine, we return the version number of this plugin (can be queried with simGetModuleName)
    // version 1 was for V-REP versions before V-REP 2.5.12
    // version 2 was for V-REP versions before V-REP 2.6.0
    // version 5 was for V-REP versions before V-REP 3.1.0
    // version 6 is for V-REP versions after V-REP 3.1.3
    // version 7 is for V-REP versions after V-REP 3.2.0 (completely rewritten)
    // version 8 is for V-REP versions after V-REP 3.3.0 (using stacks for data exchange with scripts)
    // version 9 is for V-REP versions after V-REP 3.4.0 (new API notation)
}

VREP_DLLEXPORT void v_repEnd()
{ // This is called just once, at the end of V-REP
    unloadVrepLibrary(vrepLib); // release the library
}

VREP_DLLEXPORT void* v_repMessage(int message,int* auxiliaryData,void* customData,int* replyData)
{ // This is called quite often. Just watch out for messages/events you want to handle
    // This function should not generate any error messages:
    int errorModeSaved;
    simGetIntegerParameter(sim_intparam_error_report_mode,&errorModeSaved);
    simSetIntegerParameter(sim_intparam_error_report_mode,sim_api_errormessage_ignore);
    bool left = false ,middle = false ,right = false ;
    float leftv=3.1415f,rightv=3.1415f;
    void* retVal=NULL;
    float temp = 2.0f;
    float** aux = new float*[3];
    for(int i=0;i<3;i++){
        aux[i]=new float[5];
    }
    int** auxcount=new int*[3];
    for(int i=0;i<3;i++){
        auxcount[i]=new int[5];
    }
    if (message==sim_message_eventcallback_modulehandle)
    {
        if ( (customData==NULL)||(std::string("BubbleRob").compare((char*)customData)==0) ) // is the command also meant for BubbleRob?
        {
            float dt=simGetSimulationTimeStep();
            for (unsigned int i=0;i<allBubbleRobs.size();i++)
            {
                if (allBubbleRobs[i].duration>0.0f)
                { // movement mode
                    if (simReadProximitySensor(allBubbleRobs[i].sensorHandle,NULL,NULL,NULL)>0){
                        cout << " Obstacle by Proximity Sensor " << endl;
                        allBubbleRobs[i].backMovementDuration=4.0f; // we detected an obstacle, we move backward for 3 seconds
                    }

                    if (allBubbleRobs[i].backMovementDuration>0.0f)
                    { // We move backward
                        cout << "Moving Backward" <<endl;
                        simSetJointTargetVelocity(allBubbleRobs[i].motorHandles[0],-1*leftv*allBubbleRobs[i].backRelativeVelocities[0]);
                        simSetJointTargetVelocity(allBubbleRobs[i].motorHandles[1],1*rightv*allBubbleRobs[i].backRelativeVelocities[1]);
                        allBubbleRobs[i].backMovementDuration-=dt;
                    }
                    else
                    { // We move forward

                        cout << "Moving Forward" <<endl;
                        if(simReadVisionSensor(allBubbleRobs[i].visionsensor[0],aux,auxcount)!=-1)
                             left = (aux[0][11]<0.3f);
                        if(simReadVisionSensor(allBubbleRobs[i].visionsensor[1],aux,auxcount)!=-1)
                             middle = (aux[0][11]<0.3f);
                        if(simReadVisionSensor(allBubbleRobs[i].visionsensor[2],aux,auxcount)!=-1)
                             right = (aux[0][11]<0.3f);
                          if(left)
                              leftv=leftv*0.03f;
                          if(right)
                              rightv=rightv*0.03f;
                          if(left&&right){
                              simSetJointTargetVelocity(allBubbleRobs[i].motorHandles[0],-3.0f);
                              simSetJointTargetVelocity(allBubbleRobs[i].motorHandles[1],+3.0f);
                          }
                          else
                          {
                              simSetJointTargetVelocity(allBubbleRobs[i].motorHandles[0],leftv);
                              simSetJointTargetVelocity(allBubbleRobs[i].motorHandles[1],rightv);
                          }

                    }
                    allBubbleRobs[i].duration-=dt;
                }
                else
                { // stopped mode
                    if (allBubbleRobs[i].waitUntilZero!=NULL)
                    {
                        allBubbleRobs[i].waitUntilZero[0]=0;
                        allBubbleRobs[i].waitUntilZero=NULL;
                    }
                    simSetJointTargetVelocity(allBubbleRobs[i].motorHandles[0],0.0f);
                    simSetJointTargetVelocity(allBubbleRobs[i].motorHandles[1],0.0f);
                }
            }
        }
    }

    if (message==sim_message_eventcallback_simulationended)
    { // simulation ended. Destroy all BubbleRob instances:
        allBubbleRobs.clear();
    }

    simSetIntegerParameter(sim_intparam_error_report_mode,errorModeSaved); // restore previous settings
    return(retVal);
}
