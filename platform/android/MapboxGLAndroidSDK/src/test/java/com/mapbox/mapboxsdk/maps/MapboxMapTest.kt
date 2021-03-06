package com.mapbox.mapboxsdk.maps

import com.mapbox.mapboxsdk.camera.CameraPosition
import com.mapbox.mapboxsdk.camera.CameraUpdateFactory
import com.mapbox.mapboxsdk.geometry.LatLng
import com.mapbox.mapboxsdk.geometry.LatLngBounds
import com.mapbox.mapboxsdk.style.layers.TransitionOptions
import io.mockk.*
import junit.framework.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class MapboxMapTest {

  private lateinit var mapboxMap: MapboxMap

  private lateinit var nativeMapView: NativeMap

  private lateinit var transform: Transform

  private lateinit var cameraChangeDispatcher: CameraChangeDispatcher

  @Before
  fun setup() {
    cameraChangeDispatcher = spyk()
    nativeMapView = mockk(relaxed = true)
    transform = mockk(relaxed = true)
    mapboxMap = MapboxMap(nativeMapView, transform, mockk(relaxed = true), null, null, cameraChangeDispatcher)
    every { nativeMapView.isDestroyed } returns false
    every { nativeMapView.nativePtr } returns 5
    mapboxMap.injectLocationComponent(spyk())
    mapboxMap.setStyle(Style.MAPBOX_STREETS)
    mapboxMap.onFinishLoadingStyle()
  }

  @Test
  fun testTransitionOptions() {
    val expected = TransitionOptions(100, 200)
    mapboxMap.style?.transition = expected
    verify { nativeMapView.transitionOptions = expected }
  }

  @Test
  fun testMoveCamera() {
    val callback = mockk<MapboxMap.CancelableCallback>()
    val target = LatLng(1.0, 2.0)
    val expected = CameraPosition.Builder().target(target).build()
    val update = CameraUpdateFactory.newCameraPosition(expected)
    mapboxMap.moveCamera(update, callback)
    verify { transform.moveCamera(mapboxMap, update, callback) }
  }

  @Test
  fun testMinZoom() {
    mapboxMap.setMinZoomPreference(10.0)
    verify { transform.minZoom = 10.0 }
  }

  @Test
  fun testMaxZoom() {
    mapboxMap.setMaxZoomPreference(10.0)
    verify { transform.maxZoom = 10.0 }
  }

  @Test
  fun testFpsListener() {
    val fpsChangedListener = mockk<MapboxMap.OnFpsChangedListener>()
    mapboxMap.onFpsChangedListener = fpsChangedListener
    assertEquals("Listener should match", fpsChangedListener, mapboxMap.onFpsChangedListener)
  }

  @Test
  fun testTilePrefetch() {
    mapboxMap.prefetchesTiles = true
    verify { nativeMapView.prefetchTiles = true }
  }

  @Test
  fun testCameraForLatLngBounds() {
    val bounds = LatLngBounds.Builder().include(LatLng()).include(LatLng(1.0, 1.0)).build()
    mapboxMap.setLatLngBoundsForCameraTarget(bounds)
    verify { nativeMapView.setLatLngBounds(bounds) }
  }

  @Test(expected = IllegalArgumentException::class)
  fun testAnimateCameraChecksDurationPositive() {
    mapboxMap.animateCamera(CameraUpdateFactory.newLatLng(LatLng(30.0, 30.0)), 0, null)
  }

  @Test(expected = IllegalArgumentException::class)
  fun testEaseCameraChecksDurationPositive() {
    mapboxMap.easeCamera(CameraUpdateFactory.newLatLng(LatLng(30.0, 30.0)), 0, null)
  }

  @Test
  fun testGetNativeMapPtr() {
    assertEquals(5, mapboxMap.nativeMapPtr)
  }

  @Test
  fun testNativeMapIsNotCalledOnStateSave() {
    clearMocks(nativeMapView)
    mapboxMap.onSaveInstanceState(mockk(relaxed = true))
    verify { nativeMapView wasNot Called }
  }

  @Test
  fun testCameraChangeDispatcherCleared() {
    mapboxMap.onDestroy()
    verify { cameraChangeDispatcher.onDestroy() }
  }

  @Test
  fun testStyleClearedOnDestroy() {
    val style = mockk<Style>(relaxed = true)
    val builder = mockk<Style.Builder>(relaxed = true)
    every { builder.build(nativeMapView) } returns style
    mapboxMap.setStyle(builder)

    mapboxMap.onDestroy()
    verify(exactly = 1) { style.clear() }
  }

  @Test
  fun testStyleCallbackNotCalledWhenPreviousFailed() {
    val style = mockk<Style>(relaxed = true)
    val builder = mockk<Style.Builder>(relaxed = true)
    every { builder.build(nativeMapView) } returns style
    val onStyleLoadedListener = mockk<Style.OnStyleLoaded>(relaxed = true)

    mapboxMap.setStyle(builder, onStyleLoadedListener)
    mapboxMap.onFailLoadingStyle()
    mapboxMap.setStyle(builder, onStyleLoadedListener)
    mapboxMap.onFinishLoadingStyle()
    verify(exactly = 1) { onStyleLoadedListener.onStyleLoaded(style) }
  }

  @Test
  fun testStyleCallbackNotCalledWhenPreviousNotFinished() {
    // regression test for #14337
    val style = mockk<Style>(relaxed = true)
    val builder = mockk<Style.Builder>(relaxed = true)
    every { builder.build(nativeMapView) } returns style
    val onStyleLoadedListener = mockk<Style.OnStyleLoaded>(relaxed = true)

    mapboxMap.setStyle(builder, onStyleLoadedListener)
    mapboxMap.setStyle(builder, onStyleLoadedListener)
    mapboxMap.onFinishLoadingStyle()
    verify(exactly = 1) { onStyleLoadedListener.onStyleLoaded(style) }
  }
}