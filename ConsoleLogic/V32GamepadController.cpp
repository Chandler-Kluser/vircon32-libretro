// *****************************************************************************
    // include console logic headers
    #include "V32GamepadController.hpp"
    #include "AuxiliaryFunctions.hpp"
    
    // include C/C++ headers
    #include <cstring>          // [ ANSI C ] Strings
// *****************************************************************************


namespace V32
{
    // =============================================================================
    //      CLASS: V32 GAMEPAD CONTROLLER
    // =============================================================================
    
    
    V32GamepadController::V32GamepadController()
    {
        // for all gamepad ports
        for( int Gamepad = 0; Gamepad < Constants::GamepadPorts; Gamepad++ )
        {
            // reset the gamepad
            ResetGamepad( Gamepad );
            
            // start with no connected gamepads
            RealTimeGamepadStates[ Gamepad ].Connected = false;
        }
        
        // set a known initial state
        Reset();
    }
    
    // -----------------------------------------------------------------------------
    
    bool V32GamepadController::ReadPort( int32_t LocalPort, V32Word& Result )
    {
        // check range
        if( LocalPort > INP_LastPort )
          return false;
        
        // global ports
        if( LocalPort == (int32_t)INP_LocalPorts::SelectedGamepad )
          Result.AsInteger = SelectedGamepad;
        
        // gamepad-specific ports
        V32Word* PortArray = (V32Word*)(&ProvidedGamepadStates[ SelectedGamepad ]);
        Result = PortArray[ LocalPort - 1 ];
        return true;
    }
    
    // -----------------------------------------------------------------------------
    
    bool V32GamepadController::WritePort( int32_t LocalPort, V32Word Value )
    {
        // only the active gamepad register can be written to
        if( LocalPort != (int32_t)INP_LocalPorts::SelectedGamepad )
          return false;
        
        // write the value only if the range is correct
        if( Value.AsInteger >= 0 && Value.AsInteger < Constants::GamepadPorts )
          SelectedGamepad = Value.AsInteger;
        
        return true;
    }
    
    // -----------------------------------------------------------------------------
    
    void V32GamepadController::ChangeFrame()
    {
        // first provide current states
        for( int i = 0; i < Constants::GamepadPorts; i++ )
          ProvidedGamepadStates[ i ] = RealTimeGamepadStates[ i ];
        
        // now increase all counts by 1 for next frame
        // (not including the connection indicator, which is a boolean)
        for( int Gamepad = 0; Gamepad < Constants::GamepadPorts; Gamepad++ )
        {
            int32_t* TimeCount = (int32_t*)(&RealTimeGamepadStates[ Gamepad ]);
            
            for( int i = 1; i <= 11; i++ )
            {
                if( TimeCount[i] < 0 ) TimeCount[i]--;
                else                   TimeCount[i]++;
                
                // keep values within a 1-minute range
                Clamp( TimeCount[i], -3600, 3600 );
            }
        }
    }
    
    // -----------------------------------------------------------------------------
    
    void V32GamepadController::Reset()
    {
        // set the first gamepad as selected
        SelectedGamepad = 0;
        
        // do NOT alter the state of gamepads! (their connection
        // and presses are independent of console power/resets)
    }
    
    // -----------------------------------------------------------------------------
    
    void V32GamepadController::ResetGamepad( int GamepadPort )
    {
        // reject invalid requests
        if( GamepadPort < 0 || GamepadPort >= Constants::GamepadPorts )
          return;
        
        // all times states are set to 1 minute unpressed
        int32_t* GamepadPresses = &RealTimeGamepadStates[ GamepadPort ].Left;
        
        for( int i = 0; i < 11; i++ )
        {
            *GamepadPresses = -3600;
            GamepadPresses++;
        }
        
        // copy that to the provided states
        memcpy( &ProvidedGamepadStates[ GamepadPort ], &RealTimeGamepadStates[ GamepadPort ], sizeof(GamepadState) );
    }
    
    // -----------------------------------------------------------------------------
    
    void V32GamepadController::SetGamepadConnection( int GamepadPort, bool Connected )
    {
        // reject invalid events
        if( GamepadPort < 0 || GamepadPort >= Constants::GamepadPorts )
          return;
        
        // change value
        RealTimeGamepadStates[ GamepadPort ].Connected = Connected;
        
        // on disconnection events, reset the state of all buttons and directions
        if( !Connected )
          ResetGamepad( GamepadPort );
    }
    
    // -----------------------------------------------------------------------------
    
    void V32GamepadController::SetGamepadControl( int GamepadPort, GamepadControls Control, bool Pressed )
    {
        // reject invalid events
        if( GamepadPort < 0 || GamepadPort >= Constants::GamepadPorts )
          return;
        
        // ignore controls for non connected gamepads
        if( !RealTimeGamepadStates[ GamepadPort ].Connected )
          return;
        
        // do not process redundant events
        // (otherwise times would incorrectly reset)
        int32_t* ControlStates = &RealTimeGamepadStates[ GamepadPort ].Left;
        bool WasPressed = (ControlStates[ (int)Control ] > 0);
        
        if( Pressed == WasPressed )
          return;
        
        // change value
        ControlStates[ (int)Control ] = (Pressed? 1 : -1);
        
        // when a new direction becomes pressed, ensure that
        // opposite directions can never be pressed simultaneously
        // (but again, avoid reseting time on redundancies)
        if( !Pressed ) return;
        
        switch( Control )
        {
            case GamepadControls::Left:
                if( RealTimeGamepadStates[ GamepadPort ].Right > 0 )
                  RealTimeGamepadStates[ GamepadPort ].Right = -1;
                break;
            case GamepadControls::Right:
                if( RealTimeGamepadStates[ GamepadPort ].Left > 0 )
                  RealTimeGamepadStates[ GamepadPort ].Left = -1;
                break;
            case GamepadControls::Up:
                if( RealTimeGamepadStates[ GamepadPort ].Down > 0 )
                  RealTimeGamepadStates[ GamepadPort ].Down = -1;
                break;
            case GamepadControls::Down:
                if( RealTimeGamepadStates[ GamepadPort ].Up > 0 )
                  RealTimeGamepadStates[ GamepadPort ].Up = -1;
                break;
            default: break;
        }
    }
}
