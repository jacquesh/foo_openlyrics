using System;
using System.Threading.Tasks;
using Amazon.S3;
using Amazon.S3.Model;
using Amazon.Lambda.Core;
using Amazon.Lambda.APIGatewayEvents;

[assembly: LambdaSerializer(typeof(Amazon.Lambda.Serialization.Json.JsonSerializer))]

namespace OpenLyricsMetrics
{
    public class MetricsServer
    {
        private AmazonS3Client s3 = new();

        public async Task HandleRequest(APIGatewayHttpApiV2ProxyRequest evt, ILambdaContext ctx)
        {
            if(string.IsNullOrEmpty(evt.Body))
            {
                return;
            }

            string dataToStore = evt.Body;
            const int maxBodyLength = 8192; // Truncate anything longer than 8K
            if(dataToStore.Length > maxBodyLength)
            {
                dataToStore = "TRUNCATED" + dataToStore.Substring(0, maxBodyLength);
            }

            string nowStr = DateTime.UtcNow.ToString("yyyy-MM-dd HH-mm");
            dataToStore = $"{{ \"ingest_time_utc\": \"{nowStr}\", \"metrics\": {dataToStore} }}";

            PutObjectRequest putRequest = new();
            putRequest.BucketName = "foo-openlyrics-metrics";
            putRequest.Key = Guid.NewGuid().ToString();
            putRequest.ContentBody = dataToStore;
            PutObjectResponse resp = await s3.PutObjectAsync(putRequest);
            ctx.Logger.LogInformation($"Completed writing {evt.Body.Length} characters to S3 with result: {resp.HttpStatusCode}");
        }
    }
}
