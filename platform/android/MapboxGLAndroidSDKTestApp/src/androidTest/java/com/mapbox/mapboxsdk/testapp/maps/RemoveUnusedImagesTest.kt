package com.mapbox.mapboxsdk.testapp.maps

import android.graphics.Bitmap
import android.support.test.rule.ActivityTestRule
import android.support.test.runner.AndroidJUnit4
import com.mapbox.mapboxsdk.camera.CameraUpdateFactory
import com.mapbox.mapboxsdk.geometry.LatLng
import com.mapbox.mapboxsdk.maps.MapView
import com.mapbox.mapboxsdk.maps.MapboxMap
import com.mapbox.mapboxsdk.maps.Style
import com.mapbox.mapboxsdk.testapp.R
import com.mapbox.mapboxsdk.testapp.activity.espresso.EspressoTestActivity
import org.junit.Assert.*
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import java.util.concurrent.TimeoutException

@RunWith(AndroidJUnit4::class)
class RemoveUnusedImagesTest {

  @Rule
  @JvmField
  var rule = ActivityTestRule(EspressoTestActivity::class.java)

  private lateinit var mapView: MapView
  private lateinit var mapboxMap: MapboxMap
  private val latch = CountDownLatch(1)

  @Before
  fun setup() {
    rule.runOnUiThread {
      mapView = rule.activity.findViewById(R.id.mapView)
      mapView.getMapAsync {
        mapboxMap = it;
        mapboxMap.setStyle(Style.Builder().fromJson(styleJson))
      }
    }
  }

  @Test
  fun testRemoveUnusedImagesUserProvidedListener() {
    rule.runOnUiThread {
      mapView.addOnStyleImageMissingListener {
        mapboxMap.style!!.addImage(it, Bitmap.createBitmap(512, 512,  Bitmap.Config.ARGB_8888,false))
      }

      // Remove layer and source, so that rendered tiles are no longer used, thus
      // map must notify client about unused images.
      mapView.addOnDidBecomeIdleListener {
        mapboxMap.style!!.removeLayer("icon")
        mapboxMap.style!!.removeSource("geojson")
      }

      mapView.addOnRemoveUnusedStyleImagesListener {
        assertEquals(2, it.size)
        mapboxMap.moveCamera(CameraUpdateFactory.newLatLngZoom(LatLng(0.0, 120.0), 8.0))

        // Check that images were not removed by default listener.
        mapView.addOnDidFinishRenderingFrameListener {
          assertNotNull(mapboxMap.style!!.getImage("small"))
          assertNotNull(mapboxMap.style!!.getImage("large"))
          latch.countDown()
        }
      }
    }

    if(!latch.await(5, TimeUnit.SECONDS)){
      throw TimeoutException()
    }
  }

  @Test
  fun testRemoveUnusedImagesDefaultListener() {
    rule.runOnUiThread {
      mapView.addOnStyleImageMissingListener {
        mapboxMap.style!!.addImage(it, Bitmap.createBitmap(512, 512,  Bitmap.Config.ARGB_8888,false))
      }

      // Remove layer and source, so that rendered tiles are no longer used, thus
      // map must request removal of unused images.
      mapView.addOnDidBecomeIdleListener {
        mapboxMap.style!!.removeLayer("icon")
        mapboxMap.style!!.removeSource("geojson")
        mapboxMap.moveCamera(CameraUpdateFactory.newLatLngZoom(LatLng(0.0, 120.0), 8.0))

        // Wait for the next frame and check that images were removed from the style.
        mapView.addOnDidFinishRenderingFrameListener {
          if (mapboxMap.style!!.getImage("small") == null && mapboxMap.style!!.getImage("large") == null) {
            latch.countDown()
          }
        }
      }
    }

    if(!latch.await(5, TimeUnit.SECONDS)){
      throw TimeoutException()
    }
  }

  companion object {
    private const val styleJson = """
    {
      "version": 8,
      "name": "Mapbox Streets",
      "sprite": "mapbox://sprites/mapbox/streets-v8",
      "glyphs": "mapbox://fonts/mapbox/{fontstack}/{range}.pbf",
      "sources": {
        "geojson": {
          "type": "geojson",
          "data": {
            "type": "FeatureCollection",
            "features": [
              {
                "type": "Feature",
                "properties": {
                  "image": "small"
                },
                "geometry": {
                  "type": "Point",
                  "coordinates": [
                    0,
                    0
                  ]
                }
              },
              {
                "type": "Feature",
                "properties": {
                  "image": "large"
                },
                "geometry": {
                  "type": "Point",
                  "coordinates": [
                    1,
                    1
                  ]
                }
              }
            ]
          }
        }
      },
      "layers": [{
        "id": "bg",
        "type": "background",
        "paint": {
          "background-color": "#f00"
        }
      },{
        "id": "icon",
        "type": "symbol",
        "source": "geojson",
        "layout": {
          "icon-image": ["get", "image"]
        }
      }]
    }
    """
  }
}
