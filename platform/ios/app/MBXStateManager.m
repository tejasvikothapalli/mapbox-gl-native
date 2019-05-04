#import "MBXStateManager.h"
#import <Mapbox/Mapbox.h>
#import "MBXState.h"
#import "MBXViewController.h"

@interface MBXStateManager()

@property (strong, nonatomic) MBXState *currentState;

@end

@implementation MBXStateManager

+ (instancetype) sharedManager {
    static dispatch_once_t once;
    static MBXStateManager* sharedManager;
    dispatch_once(&once, ^{
        sharedManager = [[self alloc] init];
    });

    return sharedManager;
}

- (MBXState*)currentState {
    NSMutableDictionary *mapStateDictionary = [[NSUserDefaults standardUserDefaults] objectForKey:@"mapStateKey"];


    if (mapStateDictionary == nil) {
        _currentState = nil;
    } else {
        _currentState = [[MBXState alloc] init];

        if (mapStateDictionary[MBXCamera]) {
            MGLMapCamera *unpackedCamera = [NSKeyedUnarchiver unarchiveObjectWithData:mapStateDictionary[MBXCamera]];
            _currentState.camera = unpackedCamera;
        }

        if (mapStateDictionary[MBXShowsUserLocation]) {
            _currentState.showsUserLocation = [mapStateDictionary[MBXShowsUserLocation] boolValue];
        }

        if (mapStateDictionary[MBXUserTrackingMode]) {
            _currentState.userTrackingMode = [mapStateDictionary[MBXUserTrackingMode] boolValue];
        }

        if (mapStateDictionary[MBXMapShowsHeadingIndicator]) {
            _currentState.showsUserHeadingIndicator = [mapStateDictionary[MBXMapShowsHeadingIndicator] boolValue];
        }

        if (mapStateDictionary[MBXShowsMapScale]) {
            _currentState.showsMapScale = [mapStateDictionary[MBXShowsMapScale] boolValue];
        }

        if (mapStateDictionary[MBXShowsZoomLevelOrnament]) {
            _currentState.showsZoomLevelOrnament = [mapStateDictionary[MBXShowsZoomLevelOrnament] boolValue];
        }

        if (mapStateDictionary[MBXShowsTimeFrameGraph]) {
            _currentState.showsTimeFrameGraph = [mapStateDictionary[MBXShowsTimeFrameGraph] boolValue];
        }

        if (mapStateDictionary[MBXMapFramerateMeasurementEnabled]) {
            _currentState.framerateMeasurementEnabled = [mapStateDictionary[MBXMapFramerateMeasurementEnabled] boolValue];
        }

        if (mapStateDictionary[MBXDebugMaskValue]) {
            _currentState.debugMask = ((NSNumber *)mapStateDictionary[MBXDebugMaskValue]).intValue;
        }

        if (mapStateDictionary[MBXDebugLoggingEnabled]) {
            _currentState.debugLoggingEnabled = [mapStateDictionary[MBXDebugLoggingEnabled] boolValue];
        }


        [[NSUserDefaults standardUserDefaults] synchronize];
    }

    return _currentState;
}

- (void)saveState:(MBXViewController*)mapViewController {
    MGLMapView *mapView = mapViewController.mapView;

    NSMutableDictionary *mapStateDictionary = [NSMutableDictionary dictionary];

    NSData *cameraData = [NSKeyedArchiver archivedDataWithRootObject:mapView.camera];
    [mapStateDictionary setObject:cameraData forKey:MBXCamera];
    [mapStateDictionary setValue:@(mapView.showsUserLocation) forKey:MBXShowsUserLocation];
    [mapStateDictionary setValue:@(mapView.userTrackingMode) forKey:MBXUserTrackingMode];
    [mapStateDictionary setValue:@(mapView.showsUserHeadingIndicator) forKey:MBXMapShowsHeadingIndicator];
    [mapStateDictionary setValue:@(mapView.showsScale) forKey:MBXShowsMapScale];
    [mapStateDictionary setValue:@(mapViewController.showsZoomLevelOrnament) forKey:MBXShowsZoomLevelOrnament];
    [mapStateDictionary setValue:@(mapViewController.showsTimeFrameGraph) forKey:MBXShowsTimeFrameGraph];
    [mapStateDictionary setValue:@(mapViewController.framerateMeasurementEnabled) forKey:MBXMapFramerateMeasurementEnabled];
    [mapStateDictionary setValue:@(mapView.debugMask) forKey:MBXDebugMaskValue];
    [mapStateDictionary setValue:@(mapViewController.debugLoggingEnabled) forKey:MBXDebugLoggingEnabled];

    [[NSUserDefaults standardUserDefaults] setObject:mapStateDictionary forKey:@"mapStateKey"];
    [[NSUserDefaults standardUserDefaults] synchronize];
}

- (void)resetState {
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"mapStateKey"];
}



@end
