#include <mbgl/test/util.hpp>
#include <mbgl/test/fixture_log_observer.hpp>
#include <mbgl/test/stub_style_observer.hpp>

#include <mbgl/renderer/image_manager.hpp>
#include <mbgl/renderer/image_manager_observer.hpp>
#include <mbgl/sprite/sprite_parser.hpp>
#include <mbgl/style/image_impl.hpp>
#include <mbgl/util/io.hpp>
#include <mbgl/util/image.hpp>
#include <mbgl/util/run_loop.hpp>
#include <mbgl/util/default_thread_pool.hpp>
#include <mbgl/util/string.hpp>

#include <utility>

using namespace mbgl;

TEST(ImageManager, Missing) {
    ImageManager imageManager;
    EXPECT_FALSE(imageManager.getImage("doesnotexist"));
}

TEST(ImageManager, Basic) {
    FixtureLog log;
    ImageManager imageManager;

    auto images = parseSprite(util::read_file("test/fixtures/annotations/emerald.png"),
                              util::read_file("test/fixtures/annotations/emerald.json"));
    for (auto& image : images) {
        imageManager.addImage(image->baseImpl);
    }

    auto metro = *imageManager.getPattern("metro");
    EXPECT_EQ(1, metro.tl()[0]);
    EXPECT_EQ(1, metro.tl()[1]);
    EXPECT_EQ(19, metro.br()[0]);
    EXPECT_EQ(19, metro.br()[1]);
    EXPECT_EQ(18, metro.displaySize()[0]);
    EXPECT_EQ(18, metro.displaySize()[1]);
    EXPECT_EQ(1.0f, metro.pixelRatio);
    EXPECT_EQ(imageManager.getPixelSize(), imageManager.getAtlasImage().size);

    test::checkImage("test/fixtures/image_manager/basic", imageManager.getAtlasImage());
}

TEST(ImageManager, Updates) {
    ImageManager imageManager;

    PremultipliedImage imageA({ 16, 12 });
    imageA.fill(255);
    imageManager.addImage(makeMutable<style::Image::Impl>("one", std::move(imageA), 1));

    auto a = *imageManager.getPattern("one");
    EXPECT_EQ(1, a.tl()[0]);
    EXPECT_EQ(1, a.tl()[1]);
    EXPECT_EQ(17, a.br()[0]);
    EXPECT_EQ(13, a.br()[1]);
    EXPECT_EQ(16, a.displaySize()[0]);
    EXPECT_EQ(12, a.displaySize()[1]);
    EXPECT_EQ(1.0f, a.pixelRatio);
    test::checkImage("test/fixtures/image_manager/updates_before", imageManager.getAtlasImage());

    PremultipliedImage imageB({ 5, 5 });
    imageA.fill(200);
    imageManager.updateImage(makeMutable<style::Image::Impl>("one", std::move(imageB), 1));

    auto b = *imageManager.getPattern("one");
    EXPECT_EQ(1, b.tl()[0]);
    EXPECT_EQ(1, b.tl()[1]);
    EXPECT_EQ(6, b.br()[0]);
    EXPECT_EQ(6, b.br()[1]);
    EXPECT_EQ(5, b.displaySize()[0]);
    EXPECT_EQ(5, b.displaySize()[1]);
    EXPECT_EQ(1.0f, b.pixelRatio);
    test::checkImage("test/fixtures/image_manager/updates_after", imageManager.getAtlasImage());
}

TEST(ImageManager, AddRemove) {
    FixtureLog log;
    ImageManager imageManager;

    imageManager.addImage(makeMutable<style::Image::Impl>("one", PremultipliedImage({ 16, 16 }), 2));
    imageManager.addImage(makeMutable<style::Image::Impl>("two", PremultipliedImage({ 16, 16 }), 2));
    imageManager.addImage(makeMutable<style::Image::Impl>("three", PremultipliedImage({ 16, 16 }), 2));

    imageManager.removeImage("one");
    imageManager.removeImage("two");

    EXPECT_NE(nullptr, imageManager.getImage("three"));
    EXPECT_EQ(nullptr, imageManager.getImage("two"));
    EXPECT_EQ(nullptr, imageManager.getImage("four"));
}

TEST(ImageManager, RemoveReleasesBinPackRect) {
    FixtureLog log;
    ImageManager imageManager;

    imageManager.addImage(makeMutable<style::Image::Impl>("big", PremultipliedImage({ 32, 32 }), 1));
    EXPECT_TRUE(imageManager.getImage("big"));

    imageManager.removeImage("big");

    imageManager.addImage(makeMutable<style::Image::Impl>("big", PremultipliedImage({ 32, 32 }), 1));
    EXPECT_TRUE(imageManager.getImage("big"));
    EXPECT_TRUE(log.empty());
}

class StubImageRequestor : public ImageRequestor {
public:
    StubImageRequestor(ImageManager& imageManager) : ImageRequestor(imageManager) {}

    void onImagesAvailable(ImageMap icons, ImageMap patterns, std::unordered_map<std::string, uint32_t> versionMap, uint64_t imageCorrelationID_) final {
        if (imagesAvailable && imageCorrelationID == imageCorrelationID_) imagesAvailable(icons, patterns, versionMap);
    }

    std::function<void (ImageMap, ImageMap, std::unordered_map<std::string, uint32_t>)> imagesAvailable;
    uint64_t imageCorrelationID = 0;
};

TEST(ImageManager, NotifiesRequestorWhenSpriteIsLoaded) {
    ImageManager imageManager;
    StubImageRequestor requestor(imageManager);
    bool notified = false;

    ImageManagerObserver observer;
    imageManager.setObserver(&observer);

    requestor.imagesAvailable = [&] (ImageMap, ImageMap, std::unordered_map<std::string, uint32_t>) {
        notified = true;
    };

    uint64_t imageCorrelationID = 0;
    ImageDependencies dependencies;
    dependencies.emplace("one", ImageType::Icon);
    imageManager.getImages(requestor, std::make_pair(dependencies, imageCorrelationID));
    ASSERT_FALSE(notified);

    imageManager.setLoaded(true);
    ASSERT_FALSE(notified);
    imageManager.notifyIfMissingImageAdded();
    ASSERT_TRUE(notified);
}

TEST(ImageManager, NotifiesRequestorImmediatelyIfDependenciesAreSatisfied) {
    ImageManager imageManager;
    StubImageRequestor requestor(imageManager);
    bool notified = false;

    requestor.imagesAvailable = [&] (ImageMap, ImageMap, std::unordered_map<std::string, uint32_t>) {
        notified = true;
    };

    uint64_t imageCorrelationID = 0;
    ImageDependencies dependencies;
    dependencies.emplace("one", ImageType::Icon);
    imageManager.addImage(makeMutable<style::Image::Impl>("one", PremultipliedImage({ 16, 16 }), 2));
    imageManager.getImages(requestor, std::make_pair(dependencies, imageCorrelationID));

    ASSERT_TRUE(notified);
}


class StubImageManagerObserver : public ImageManagerObserver {
    public:
    int count = 0;
    std::function<void (const std::string&)> imageMissing = [](const std::string&){};
    void onStyleImageMissing(const std::string& id, std::function<void()> done) override {
        count++;
        imageMissing(id);
        done();
    }

    std::function<void (std::vector<std::string>)> removeUnusedStyleImages = [](std::vector<std::string>){};
    void onRemoveUnusedStyleImages(std::vector<std::string> unusedImageIDs) override {
        removeUnusedStyleImages(std::move(unusedImageIDs));
    }
};

TEST(ImageManager, OnStyleImageMissingBeforeSpriteLoaded) {
    ImageManager imageManager;
    StubImageRequestor requestor(imageManager);
    StubImageManagerObserver observer;

    imageManager.setObserver(&observer);

    bool notified = false;

    requestor.imagesAvailable = [&] (ImageMap, ImageMap, std::unordered_map<std::string, uint32_t>) {
        notified = true;
    };

    uint64_t imageCorrelationID = 0;
    ImageDependencies dependencies;
    dependencies.emplace("pre", ImageType::Icon);
    imageManager.getImages(requestor, std::make_pair(dependencies, imageCorrelationID));

    EXPECT_EQ(observer.count, 0);
    ASSERT_FALSE(notified);

    imageManager.setLoaded(true);

    EXPECT_EQ(observer.count, 1);
    ASSERT_FALSE(notified);

    imageManager.notifyIfMissingImageAdded();

    EXPECT_EQ(observer.count, 1);
    ASSERT_TRUE(notified);

}

TEST(ImageManager, OnStyleImageMissingAfterSpriteLoaded) {
    ImageManager imageManager;
    StubImageRequestor requestor(imageManager);
    StubImageManagerObserver observer;

    imageManager.setObserver(&observer);

    bool notified = false;

    requestor.imagesAvailable = [&] (ImageMap, ImageMap, std::unordered_map<std::string, uint32_t>) {
        notified = true;
    };

    EXPECT_EQ(observer.count, 0);
    ASSERT_FALSE(notified);

    imageManager.setLoaded(true);

    uint64_t imageCorrelationID = 0;
    ImageDependencies dependencies;
    dependencies.emplace("after", ImageType::Icon);
    imageManager.getImages(requestor, std::make_pair(dependencies, imageCorrelationID));

    EXPECT_EQ(observer.count, 1);
    ASSERT_FALSE(notified);

    imageManager.notifyIfMissingImageAdded();

    EXPECT_EQ(observer.count, 1);
    ASSERT_TRUE(notified);
}

TEST(ImageManager, RemoveUnusedStyleImages) {
    ImageManager imageManager;
    StubImageManagerObserver observer;
    imageManager.setObserver(&observer);
    imageManager.setLoaded(true);

    observer.imageMissing = [&imageManager] (const std::string& id) {
        if (id == "1024px") {
            imageManager.addImage(makeMutable<style::Image::Impl>(id, PremultipliedImage({ 1024, 1024 }), 1));
        } else {
            imageManager.addImage(makeMutable<style::Image::Impl>(id, PremultipliedImage({ 16, 16 }), 1));
        }
    };

    observer.removeUnusedStyleImages = [&imageManager](std::vector<std::string> ids) {
        for (const auto& id : ids) {
            assert(imageManager.getImage(id));
            imageManager.removeImage(id);
        }
    };

    // Style sprite.
    imageManager.addImage(makeMutable<style::Image::Impl>("sprite", PremultipliedImage({ 16, 16 }), 1));

    // Single requestor
    {
        std::unique_ptr<StubImageRequestor> requestor = std::make_unique<StubImageRequestor>(imageManager);
        imageManager.getImages(*requestor, std::make_pair(ImageDependencies{{"missing", ImageType::Icon}}, 0ull));
        EXPECT_EQ(observer.count, 1);
        ASSERT_FALSE(imageManager.getImage("missing") == nullptr);
    }

    // Within cache limits, no need to notify client.
    imageManager.checkCacheSizeReduceMemoryUse();
    ASSERT_FALSE(imageManager.getImage("missing") == nullptr);

    // Simulate OOM case, forces client notification to be issued.
    imageManager.reduceMemoryUse();
    ASSERT_TRUE(imageManager.getImage("missing") == nullptr);
    ASSERT_FALSE(imageManager.getImage("sprite") == nullptr);

    // Single requestor, exceed cache size limit.
    {
        std::unique_ptr<StubImageRequestor> requestor = std::make_unique<StubImageRequestor>(imageManager);
        imageManager.getImages(*requestor, std::make_pair(ImageDependencies{{"1024px", ImageType::Icon}}, 0ull));
        EXPECT_EQ(observer.count, 2);
        ASSERT_FALSE(imageManager.getImage("1024px") == nullptr);
    }

    // Over cache limits, need to notify client.
    imageManager.checkCacheSizeReduceMemoryUse();
    ASSERT_TRUE(imageManager.getImage("1024px") == nullptr);
    ASSERT_FALSE(imageManager.getImage("sprite") == nullptr);

    // Multiple requestors
    {
        std::unique_ptr<StubImageRequestor> requestor1 = std::make_unique<StubImageRequestor>(imageManager);
        std::unique_ptr<StubImageRequestor> requestor2 = std::make_unique<StubImageRequestor>(imageManager);
        imageManager.getImages(*requestor1, std::make_pair(ImageDependencies{{"missing", ImageType::Icon}}, 0ull));
        imageManager.getImages(*requestor2, std::make_pair(ImageDependencies{{"missing", ImageType::Icon}}, 1ull));
        EXPECT_EQ(observer.count, 3);
        ASSERT_FALSE(imageManager.getImage("missing") == nullptr);
    }

    // Reduce memory usage and check that unused image was deleted when all requestors are destructed.
    imageManager.checkCacheSizeReduceMemoryUse();
    ASSERT_FALSE(imageManager.getImage("missing") == nullptr);
    imageManager.reduceMemoryUse();
    ASSERT_TRUE(imageManager.getImage("missing") == nullptr);

    // Multiple requestors, check that image resource is not destroyed if there is at least 1 requestor that uses it.
    std::unique_ptr<StubImageRequestor> requestor = std::make_unique<StubImageRequestor>(imageManager);
    {
        std::unique_ptr<StubImageRequestor> requestor1 = std::make_unique<StubImageRequestor>(imageManager);
        imageManager.getImages(*requestor, std::make_pair(ImageDependencies{{"missing", ImageType::Icon}, {"1024px", ImageType::Icon}}, 0ull));
        imageManager.getImages(*requestor1, std::make_pair(ImageDependencies{{"missing", ImageType::Icon}}, 1ull));
        EXPECT_EQ(observer.count, 5);
        ASSERT_FALSE(imageManager.getImage("missing") == nullptr);
        ASSERT_FALSE(imageManager.getImage("1024px") == nullptr);
    }

    // Reduce memory usage and check that requested image is not destructed.
    imageManager.checkCacheSizeReduceMemoryUse();
    ASSERT_FALSE(imageManager.getImage("missing") == nullptr);
    ASSERT_FALSE(imageManager.getImage("1024px") == nullptr);

    // Release last requestor and check if resource was released when cache size is over the limit.
    requestor.reset();
    imageManager.checkCacheSizeReduceMemoryUse();
    ASSERT_TRUE(imageManager.getImage("missing") == nullptr);
    ASSERT_TRUE(imageManager.getImage("1024px") == nullptr);
    ASSERT_FALSE(imageManager.getImage("sprite") == nullptr);
}
