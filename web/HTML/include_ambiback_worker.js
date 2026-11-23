self.addEventListener('install', function(e) {
  console.log("install: " + JSON.stringify(e));
});

self.addEventListener('fetch', function(e) {
  // console.log("fetch: " + e.request.url);
  e.respondWith(
    caches.match(e.request).then(function(response) {
      return response || fetch(e.request);
    })
  );
});
